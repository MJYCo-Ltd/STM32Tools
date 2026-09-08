/* Exercise transaction boundaries and error propagation with storage I/O
 * doubles. This does not replace power-cut testing of the real NOR backend. */
#include <assert.h>
#include <string.h>
#include <Flash/DualBankStore.h>
#include <Flash/storage_record.h>
#include <Flash/storage_bank.h>

typedef struct {
  unsigned held, begins, ends, reads, writes, switches;
  Storage_Status begin_status, read_status, append_status, switch_status;
  Storage_Status payload_status;
  uint32_t values[2], lengths[2], sequences[2];
} FakeStorage;
static FakeStorage io;

static Storage_Status Begin(void *ctx)
{
  assert(ctx == &io && io.held == 0U);
  ++io.begins;
  if (io.begin_status == STORAGE_OK) io.held = 1U;
  return io.begin_status;
}
static void End(void *ctx)
{
  assert(ctx == &io && io.held == 1U);
  io.held = 0U;
  ++io.ends;
}
static uint8_t Valid(const void *payload) { return *(const uint32_t *)payload == 42U; }
int Storage_SeqIsNewer(uint32_t a, uint32_t b) { return (int32_t)(a - b) > 0; }

Storage_Status StorageRecord_FindLatest(const StoragePartitionMap *map,
    uint32_t partition, uint32_t offset, uint32_t size, StorageRecordLoc *loc,
    void *payload, uint32_t capacity)
{
  (void)map; (void)partition; (void)size;
  assert(io.held && (payload == NULL || capacity == sizeof(uint32_t)));
  ++io.reads;
  memset(loc, 0, sizeof(*loc));
  const uint32_t bank = offset == 0U ? 0U : 1U;
  loc->offset = offset;
  loc->payload_length = io.lengths[bank];
  loc->sequence = io.sequences[bank];
  if (payload != NULL) *(uint32_t *)payload = io.values[bank];
  return io.read_status;
}
Storage_Status StorageRecord_ReadPayload(const StoragePartitionMap *map,
    uint32_t partition, uint32_t offset, void *payload, uint32_t capacity,
    uint32_t *length)
{
  (void)map; (void)partition;
  assert(io.held && capacity == sizeof(uint32_t));
  const uint32_t bank = offset == 0U ? 0U : 1U;
  *(uint32_t *)payload = io.values[bank];
  if (length != NULL) *length = io.lengths[bank];
  return io.payload_status;
}
Storage_Status StorageRecord_Append(const StoragePartitionMap *map,
    uint32_t partition, uint32_t offset, uint32_t size, uint32_t sequence,
    const void *payload, uint32_t length, uint32_t *written)
{
  (void)map; (void)partition; (void)offset; (void)size; (void)sequence;
  (void)payload; (void)length; (void)written;
  assert(io.held);
  ++io.writes;
  return io.append_status;
}
Storage_Status StorageBank_Switch(const StoragePartitionMap *map,
    uint32_t partition, uint32_t dest, uint32_t dest_size, uint32_t old,
    uint32_t old_size, uint32_t sequence, const void *payload, uint32_t length,
    uint32_t extra, uint32_t extra_size)
{
  (void)map; (void)partition; (void)dest; (void)dest_size; (void)old;
  (void)old_size; (void)sequence; (void)payload; (void)length;
  (void)extra; (void)extra_size;
  assert(io.held);
  ++io.switches;
  return io.switch_status;
}

int main(void)
{
  DualBankStore store;
  StoragePartitionMap map = {0};
  DualBankTransaction transaction = {&io, Begin, End};
  uint32_t a, b, payload = 42U;
  io.values[0] = io.values[1] = 42U;
  io.lengths[0] = io.lengths[1] = sizeof(uint32_t);
  io.sequences[0] = 1U;
  io.sequences[1] = 2U;
  io.read_status = STORAGE_ERR_NOT_FOUND;
  assert(DualBankStore_Init(&store, &transaction, &map, 0, 0, 4096, 4096,
      sizeof(payload), Valid, &a, &b) == STORAGE_ERR_NOT_FOUND);
  assert(io.reads == 2U && io.begins == 1U && io.ends == 1U && !io.held);
  /* Init copies callbacks: its caller may use a stack-local descriptor. */
  transaction.begin = NULL;
  io.begin_status = STORAGE_ERR_BUSY;
  assert(DualBankStore_Save(&store, &payload) == STORAGE_ERR_BUSY);
  assert(io.writes == 0U && io.ends == 1U);
  assert(store.next_sequence == 1U && !store.have_snapshot);

  io.begin_status = STORAGE_OK;
  io.append_status = STORAGE_ERR_NO_SPACE;
  io.switch_status = STORAGE_ERR_IO;
  assert(DualBankStore_Save(&store, &payload) == STORAGE_ERR_IO);
  assert(io.switches == 1U && io.ends == 2U && !io.held);
  assert(store.active_bank == 0U && store.next_sequence == 1U);

  io.switch_status = STORAGE_OK;
  assert(DualBankStore_Save(&store, &payload) == STORAGE_OK);
  assert(store.active_bank == 1U && store.next_sequence == 2U && store.have_snapshot);
  io.read_status = STORAGE_ERR_IO;
  assert(DualBankStore_Load(&store, &a) == STORAGE_ERR_IO);
  assert(io.ends == 4U && !io.held);
  io.read_status = STORAGE_OK;
  assert(DualBankStore_Load(&store, &a) == STORAGE_OK && a == payload);
  assert(io.ends == 5U && !io.held);
  transaction.begin = Begin;
  io.values[0] = 99U; /* Older incompatible bank must not poison newest. */
  io.lengths[0] = 2U;
  assert(DualBankStore_Init(&store, &transaction, &map, 0, 0, 4096, 4096,
      sizeof(payload), Valid, &a, &b) == STORAGE_OK);
  assert(store.active_bank == 1U && store.next_sequence == 3U);
  io.values[0] = 42U;
  io.lengths[0] = 4U;
  io.values[1] = 99U; /* Never restore stale state when newest is invalid. */
  assert(DualBankStore_Init(&store, &transaction, &map, 0, 0, 4096, 4096,
      sizeof(payload), Valid, &a, &b) == STORAGE_ERR_STATE && !store.have_snapshot);
  io.values[1] = 42U;
  io.lengths[1] = 2U;
  assert(DualBankStore_Init(&store, &transaction, &map, 0, 0, 4096, 4096,
      sizeof(payload), Valid, &a, &b) == STORAGE_ERR_STATE);
  io.lengths[1] = 4U;
  io.payload_status = STORAGE_ERR_IO;
  assert(DualBankStore_Init(&store, &transaction, &map, 0, 0, 4096, 4096,
      sizeof(payload), Valid, &a, &b) == STORAGE_ERR_IO);
  assert(!io.held && io.begins == io.ends + 1U);
  return 0;
}
