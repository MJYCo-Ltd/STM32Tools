#include <Flash/DurableRecordQueue.h>
#include <Common.h>
#include <stddef.h>
#include <string.h>

#define SECTOR_SIZE 4096U
#define SECTOR_SLOTS (SECTOR_SIZE / DURABLE_RECORD_SIZE)
#define COMMITTED 0x434F4D31U
#define ACKED 0U

typedef struct {
  uint32_t magic, epoch, sequence;
  uint8_t payload[40];
  uint32_t crc, commit, ack;
} DiskRecord;
typedef struct { uint32_t schema, epoch; } Epoch;
_Static_assert(sizeof(DiskRecord) == DURABLE_RECORD_SIZE, "durable record ABI");
_Static_assert(offsetof(DiskRecord, crc) == 52U, "durable CRC coverage");
_Static_assert(SECTOR_SLOTS == DURABLE_SECTOR_SLOTS, "durable sector geometry");

static Storage_Status Begin(DurableRecordQueue *q)
{
  if (q->transaction.begin == NULL || q->transaction.end == NULL)
    return STORAGE_ERR_STATE;
  return q->transaction.begin(q->transaction.ctx);
}
#define QUEUE_LOCKED_CALL(expression) do { \
  Storage_Status operation_status = Begin(q); \
  if (operation_status != STORAGE_OK) return operation_status; \
  operation_status = (expression); \
  q->transaction.end(q->transaction.ctx); \
  return operation_status; \
} while (0)

enum { FREE, PENDING, USED, CORRUPT };
enum { HAS_FREE = 1U, HAS_PENDING = 2U, HAS_CORRUPT = 4U, QUARANTINED = 8U };
static uint8_t CacheState(const DurableSectorCache *c, uint32_t slot)
{ return (c->states[(slot % SECTOR_SLOTS) / 4U] >> ((slot % 4U) * 2U)) & 3U; }
static void CacheSet(DurableSectorCache *c, uint32_t slot, uint8_t state)
{
  const uint32_t shift = (slot % 4U) * 2U;
  uint8_t *byte = &c->states[(slot % SECTOR_SLOTS) / 4U];
  *byte = (*byte & (uint8_t)~(3U << shift)) | (uint8_t)(state << shift);
}
static uint8_t Summary(const DurableSectorCache *c)
{
  uint8_t flags = 0U;
  for (uint32_t i = 0U; i < SECTOR_SLOTS; ++i) {
    const uint8_t state = CacheState(c, i);
    if (state == FREE) flags |= HAS_FREE;
    if (state == PENDING) flags |= HAS_PENDING;
    if (state == CORRUPT) flags |= HAS_CORRUPT;
  }
  return flags;
}
static uint32_t CacheFree(const DurableSectorCache *c)
{
  uint32_t n = 0U;
  for (uint32_t i = 0U; i < SECTOR_SLOTS; ++i) if (CacheState(c, i) == FREE) ++n;
  return n;
}
static void SetSlotState(DurableRecordQueue *q, DurableSectorCache *c,
                         uint32_t slot, uint8_t state)
{
  const uint8_t quarantine = q->config->sectors[c->sector] & QUARANTINED;
  if (!quarantine) {
    if (CacheState(c, slot) == FREE) --q->free_slots;
    if (state == FREE) ++q->free_slots;
  }
  CacheSet(c, slot, state);
  q->config->sectors[c->sector] = Summary(c) | quarantine;
  if (quarantine) q->config->sectors[c->sector] &= (uint8_t)~HAS_FREE;
}
/* Uncertain write/erase stays isolated even after cache eviction. Only Init
 * may reclassify its persistent markers; other pending sectors remain usable. */
static void Quarantine(DurableRecordQueue *q, DurableSectorCache *c)
{
  if (!(q->config->sectors[c->sector] & QUARANTINED))
    q->free_slots -= CacheFree(c);
  q->config->sectors[c->sector] = (Summary(c) | QUARANTINED) & (uint8_t)~HAS_FREE;
}
static uint8_t ValidEpoch(const void *raw)
{
  const Epoch *e = raw;
  return e->schema == 1U && e->epoch != 0U;
}
static uint32_t Offset(const DurableRecordQueue *q, uint32_t slot)
{
  return q->config->data_offset + slot * DURABLE_RECORD_SIZE;
}
static Storage_Status Read(const DurableRecordQueue *q, uint32_t slot, DiskRecord *r)
{
  return Storage_Read(q->map, q->partition, Offset(q, slot), r, sizeof(*r));
}
static uint8_t Valid(const DurableRecordQueue *q, const DiskRecord *r)
{
  return r->magic == q->config->magic && r->epoch != 0U && r->sequence != 0U &&
      r->commit == COMMITTED && r->crc == CalCRC32((const uint8_t *)r, offsetof(DiskRecord, crc));
}
static uint8_t Erased(const DiskRecord *r)
{
  const uint8_t *p = (const uint8_t *)r;
  for (uint32_t i = 0U; i < sizeof(*r); ++i) if (p[i] != 0xFFU) return 0U;
  return 1U;
}
static uint8_t Classify(const DurableRecordQueue *q, const DiskRecord *r)
{
  if (Erased(r)) return FREE;
  if (Valid(q, r)) return r->ack == ACKED ? USED : PENDING;
  return r->commit == COMMITTED ? CORRUPT : USED;
}
static Storage_Status LoadSector(DurableRecordQueue *q, uint32_t sector,
                                  DurableSectorCache **out)
{
  DurableSectorCache next = {0};
  DiskRecord records[4];
  Storage_Status st;
  for (uint32_t i = 0U; i < DURABLE_CACHE_COUNT; ++i) {
    if (q->cache[i].valid && q->cache[i].sector == sector) {
      *out = &q->cache[i];
      return STORAGE_OK;
    }
  }
  next.sector = sector;
  for (uint32_t i = 0U; i < SECTOR_SLOTS; i += 4U) {
    st = Storage_Read(q->map, q->partition,
        Offset(q, sector * SECTOR_SLOTS + i), records, sizeof(records));
    if (st != STORAGE_OK) return st;
    for (uint32_t j = 0U; j < 4U; ++j) CacheSet(&next, i + j, Classify(q, &records[j]));
  }
  next.valid = 1U;
  /* Persistent ACKs rebuild cache flags; ack_pending separately pins uncertain
   * caller results. Failed append/erase sectors retain their quarantine. */
  const uint8_t quarantine = q->config->sectors[sector] & QUARANTINED;
  q->config->sectors[sector] = Summary(&next) | quarantine;
  if (quarantine) q->config->sectors[sector] &= (uint8_t)~HAS_FREE;
  *out = &q->cache[q->next_cache];
  **out = next;
  q->next_cache = (q->next_cache + 1U) % DURABLE_CACHE_COUNT;
  return STORAGE_OK;
}
static Storage_Status FindSlot(DurableRecordQueue *q, uint32_t start,
                               uint8_t wanted, uint32_t *slot,
                               DurableSectorCache **cache)
{
  const uint8_t flag = wanted == FREE ? HAS_FREE : HAS_PENDING;
  const uint32_t sectors = q->config->capacity / SECTOR_SLOTS;
  const uint32_t first = start / SECTOR_SLOTS;
  for (uint32_t pass = 0U; pass <= sectors; ++pass) {
    const uint32_t sector = (first + pass) % sectors;
    if (!(q->config->sectors[sector] & flag)) continue;
    Storage_Status st = LoadSector(q, sector, cache);
    if (st != STORAGE_OK) return st;
    if (!(q->config->sectors[sector] & flag)) return STORAGE_ERR_BUSY;
    const uint32_t begin = pass == 0U ? start % SECTOR_SLOTS : 0U;
    const uint32_t end = pass == sectors ? start % SECTOR_SLOTS : SECTOR_SLOTS;
    for (uint32_t i = begin; i < end; ++i) {
      if (CacheState(*cache, i) == wanted) {
        *slot = sector * SECTOR_SLOTS + i;
        return STORAGE_OK;
      }
    }
    if (pass != 0U && pass != sectors) return STORAGE_ERR_STATE;
  }
  return wanted == FREE ? STORAGE_ERR_NO_SPACE : STORAGE_ERR_NOT_FOUND;
}
static Storage_Status InitLocked(DurableRecordQueue *q)
{
  DualBankStore epochs;
  Epoch a, b, next, verified;
  DiskRecord r;
  uint32_t max_epoch = 0U;
  Storage_Status st;
  q->free_slots = 0U;
  q->next_cache = 0U;
  q->ack_pending = 0U;
  q->ack_slot = q->ack_epoch = q->ack_sequence = 0U;
  memset(q->cache, 0, sizeof(q->cache));
  memset(q->config->sectors, 0, DURABLE_INDEX_BYTES(q->config->capacity));
  for (uint32_t i = 0U; i < q->config->capacity; ++i) {
    if ((i % SECTOR_SLOTS) == 0U) StorageBackend_Poll(q->map->backend);
    st = Read(q, i, &r);
    if (st != STORAGE_OK) return st;
    const uint8_t state = Classify(q, &r);
    uint8_t *summary = &q->config->sectors[i / SECTOR_SLOTS];
    if (state == FREE) { *summary |= HAS_FREE; ++q->free_slots; }
    if (state == PENDING) *summary |= HAS_PENDING;
    if (state == CORRUPT) *summary |= HAS_CORRUPT;
    if (Valid(q, &r)) {
      if (r.epoch > max_epoch) max_epoch = r.epoch;
    } else if (state != FREE && q->config->report_error != NULL) {
      q->config->report_error(state == CORRUPT ? STORAGE_ERR_CRC : STORAGE_ERR_STATE);
    }
  }
  st = DualBankStore_Init(&epochs, &q->transaction, q->map, q->partition,
      q->config->epoch_a, q->config->epoch_b, SECTOR_SIZE,
      sizeof(Epoch), ValidEpoch, &a, &b);
  if (st != STORAGE_OK && st != STORAGE_ERR_NOT_FOUND) return st;
  if (st == STORAGE_OK) {
    st = DualBankStore_Load(&epochs, &a);
    if (st != STORAGE_OK) return st;
    if (a.epoch > max_epoch) max_epoch = a.epoch;
  }
  if (max_epoch == UINT32_MAX) return STORAGE_ERR_STATE;
  next.schema = 1U;
  next.epoch = max_epoch + 1U;
  st = DualBankStore_Save(&epochs, &next);
  if (st != STORAGE_OK) return st;
  st = DualBankStore_Load(&epochs, &verified);
  if (st != STORAGE_OK) return st;
  if (verified.epoch != next.epoch) return STORAGE_ERR_STATE;
  q->epoch = next.epoch;
  q->sequence = q->write_slot = q->read_slot = 0U;
  q->ready = 1U;
  return STORAGE_OK;
}
Storage_Status DurableRecordQueue_InitEx(DurableRecordQueue *q,
    const DurableRecordQueueConfig *config, const StoragePartitionMap *map,
    uint32_t partition, const DualBankTransaction *transaction)
{
  if (q == NULL) return STORAGE_ERR_PARAM;
  q->ready = 0U;
  if (config == NULL || config->sectors == NULL || !config->capacity ||
      config->capacity % SECTOR_SLOTS || config->data_offset % SECTOR_SIZE ||
      config->capacity > (UINT32_MAX - config->data_offset) / DURABLE_RECORD_SIZE ||
      map == NULL || transaction == NULL || transaction->begin == NULL ||
      transaction->end == NULL) return STORAGE_ERR_PARAM;
  q->config = config;
  q->map = map;
  q->partition = partition;
  q->transaction = *transaction;
  QUEUE_LOCKED_CALL(InitLocked(q));
}
static Storage_Status AppendLocked(DurableRecordQueue *q, const void *record)
{
  DiskRecord r, verified;
  uint32_t slot, commit = COMMITTED;
  DurableSectorCache *cache;
  Storage_Status st;
  if (!q->ready || q->sequence == UINT32_MAX) return STORAGE_ERR_STATE;
  st = FindSlot(q, q->write_slot, FREE, &slot, &cache);
  if (st != STORAGE_OK) return st;
  memcpy(&r, record, sizeof(r));
  r.magic = q->config->magic;
  r.epoch = q->epoch;
  r.sequence = ++q->sequence;
  r.crc = CalCRC32((const uint8_t *)&r, offsetof(DiskRecord, crc));
  r.commit = r.ack = UINT32_MAX;
  SetSlotState(q, cache, slot, CORRUPT);
  st = Storage_Write(q->map, q->partition, Offset(q, slot), &r, offsetof(DiskRecord, commit));
  if (st == STORAGE_OK) st = Read(q, slot, &verified);
  if (st == STORAGE_OK && memcmp(&r, &verified, sizeof(r)) != 0) st = STORAGE_ERR_CRC;
  if (st == STORAGE_OK) st = Storage_Write(q->map, q->partition,
      Offset(q, slot) + offsetof(DiskRecord, commit), &commit, sizeof(commit));
  if (st == STORAGE_OK) st = Read(q, slot, &verified);
  if (st == STORAGE_OK && !Valid(q, &verified)) st = STORAGE_ERR_CRC;
  if (st != STORAGE_OK) { Quarantine(q, cache); return st; }
  SetSlotState(q, cache, slot, PENDING);
  q->write_slot = (slot + 1U) % q->config->capacity;
  return STORAGE_OK;
}
Storage_Status DurableRecordQueue_Append(DurableRecordQueue *q, const void *record)
{
  if (q == NULL || record == NULL) return STORAGE_ERR_PARAM;
  QUEUE_LOCKED_CALL(AppendLocked(q, record));
}
static Storage_Status PeekLocked(DurableRecordQueue *q, void *out)
{
  DiskRecord r;
  uint32_t slot;
  DurableSectorCache *cache;
  Storage_Status st;
  if (!q->ready) return STORAGE_ERR_STATE;
  st = FindSlot(q, q->read_slot, PENDING, &slot, &cache);
  if (st != STORAGE_OK) return st;
  st = Read(q, slot, &r);
  if (st != STORAGE_OK) return st;
  if (!Valid(q, &r)) {
    SetSlotState(q, cache, slot, CORRUPT);
    Quarantine(q, cache);
    return STORAGE_ERR_CRC;
  }
  q->read_slot = slot;
  memcpy(out, &r, sizeof(r));
  return STORAGE_OK;
}
Storage_Status DurableRecordQueue_Peek(DurableRecordQueue *q, void *record)
{
  if (q == NULL || record == NULL) return STORAGE_ERR_PARAM;
  QUEUE_LOCKED_CALL(PeekLocked(q, record));
}
static Storage_Status AckLocked(DurableRecordQueue *q, uint32_t epoch, uint32_t sequence)
{
  DiskRecord r;
  uint32_t ack = ACKED, slot;
  Storage_Status st;
  DurableSectorCache *cache;
  if (!q->ready) return STORAGE_ERR_STATE;
  if (q->ack_pending && (epoch != q->ack_epoch || sequence != q->ack_sequence))
    return STORAGE_ERR_NOT_FOUND;
  slot = q->ack_pending ? q->ack_slot : q->read_slot;
  st = LoadSector(q, slot / SECTOR_SLOTS, &cache);
  if (st != STORAGE_OK) return st;
  st = Read(q, slot, &r);
  if (st != STORAGE_OK) return st;
  if (!Valid(q, &r) || r.epoch != epoch || r.sequence != sequence)
    return STORAGE_ERR_NOT_FOUND;
  if (r.ack != ACKED) {
    q->ack_slot = slot;
    q->ack_epoch = epoch;
    q->ack_sequence = sequence;
    q->ack_pending = 1U;
    st = Storage_Write(q->map, q->partition,
        Offset(q, slot) + offsetof(DiskRecord, ack), &ack, sizeof(ack));
    if (st != STORAGE_OK) return st;
    st = Read(q, slot, &r);
    if (st != STORAGE_OK) return st;
    if (!Valid(q, &r) || r.epoch != epoch || r.sequence != sequence || r.ack != ACKED)
      return STORAGE_ERR_STATE;
  }
  SetSlotState(q, cache, slot, USED);
  if (q->read_slot == slot) q->read_slot = (slot + 1U) % q->config->capacity;
  q->ack_pending = 0U;
  return STORAGE_OK;
}
Storage_Status DurableRecordQueue_Acknowledge(DurableRecordQueue *q, uint32_t epoch, uint32_t sequence)
{
  if (q == NULL) return STORAGE_ERR_PARAM;
  QUEUE_LOCKED_CALL(AckLocked(q, epoch, sequence));
}
static Storage_Status ReclaimLocked(DurableRecordQueue *q)
{
  DiskRecord r;
  Storage_Status st;
  if (!q->ready) return STORAGE_ERR_STATE;
  if (q->free_slots >= SECTOR_SLOTS) return STORAGE_OK;
  const uint32_t sectors = q->config->capacity / SECTOR_SLOTS;
  for (uint32_t i = 0U; i < sectors; ++i) {
    const uint32_t sector = (q->write_slot / SECTOR_SLOTS + i) % sectors;
    if (q->ack_pending && sector == q->ack_slot / SECTOR_SLOTS) continue;
    if (q->config->sectors[sector] & (HAS_PENDING | HAS_CORRUPT | QUARANTINED)) continue;
    DurableSectorCache *cache;
    st = LoadSector(q, sector, &cache);
    if (st != STORAGE_OK) return st;
    for (uint32_t j = 0U; j < SECTOR_SLOTS; ++j) {
      st = Read(q, sector * SECTOR_SLOTS + j, &r);
      if (st != STORAGE_OK) return st;
      if (Classify(q, &r) == PENDING || Classify(q, &r) == CORRUPT) {
        Quarantine(q, cache);
        return STORAGE_ERR_STATE;
      }
    }
    Quarantine(q, cache);
    st = Storage_EraseSector(q->map, q->partition, Offset(q, sector * SECTOR_SLOTS));
    if (st != STORAGE_OK) return st;
    for (uint32_t j = 0U; j < SECTOR_SLOTS; ++j) {
      st = Read(q, sector * SECTOR_SLOTS + j, &r);
      if (st != STORAGE_OK) return st;
      if (!Erased(&r)) return STORAGE_ERR_STATE;
    }
    memset(cache->states, 0, sizeof(cache->states));
    q->config->sectors[sector] = HAS_FREE;
    q->free_slots += SECTOR_SLOTS;
    return STORAGE_OK;
  }
  return STORAGE_ERR_NO_SPACE;
}
Storage_Status DurableRecordQueue_ReclaimOne(DurableRecordQueue *q)
{
  if (q == NULL) return STORAGE_ERR_PARAM;
  QUEUE_LOCKED_CALL(ReclaimLocked(q));
}
