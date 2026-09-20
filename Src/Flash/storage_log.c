#include "Flash/storage_log.h"

#include "Flash/nor_flash.h"
#include "Flash/storage_commit.h"
#include "Flash/storage_crc.h"

#include <stddef.h>
#include <string.h>

static uint32_t SectorCount(const StorageLog *log)
{
  return log->region_size / NOR_FLASH_SECTOR_SIZE;
}

static uint32_t SectorBase(const StorageLog *log, uint32_t index)
{
  return log->region_offset + (index * NOR_FLASH_SECTOR_SIZE);
}

static Storage_Status SectorHeaderCrc(const StorageLogSectorHeader *header,
                                      uint32_t *crc_out)
{
  return Storage_ComputeCrcExcludingCommit(
      header, sizeof(*header), offsetof(StorageLogSectorHeader, header_crc32),
      crc_out);
}

static Storage_Status ReadSectorHeader(const StorageLog *log, uint32_t index,
                                       StorageLogSectorHeader *header)
{
  return Storage_Read(log->map, log->partition, SectorBase(log, index), header,
                      sizeof(*header));
}

static int SectorHeaderValid(const StorageLogSectorHeader *header)
{
  uint32_t header_crc;

  if ((header->magic != STORAGE_LOG_SECTOR_MAGIC) ||
      (header->commit_marker != STORAGE_COMMIT_MARKER)) {
    return 0;
  }
  return ((SectorHeaderCrc(header, &header_crc) == STORAGE_OK) &&
          (header_crc == header->header_crc32))
             ? 1
             : 0;
}

static Storage_Status CommitSectorHeader(StorageLog *log, uint32_t index,
                                         uint32_t sector_sequence,
                                         uint32_t erase_count)
{
  StorageLogSectorHeader header;
  uint32_t base = SectorBase(log, index);

  memset(&header, 0, sizeof(header));
  header.magic = STORAGE_LOG_SECTOR_MAGIC;
  header.sector_sequence = sector_sequence;
  header.erase_count = erase_count;
  return Storage_CommitObject(log->map, log->partition, base, &header,
                              sizeof(header),
                              offsetof(StorageLogSectorHeader, header_crc32));
}

/* The V1 header CRC excludes commit_marker. A zero marker is an acknowledged
 * log record. A proper subset of COMMIT is an interrupted ACK: retry delivery.
 * An interrupted original commit is a superset, and is NEVER made pending. */
static Storage_Status ReadLogRecord(const StorageLog *log, uint32_t offset,
    uint32_t end, StorageRecordHeader *h, StorageRecordLoc *loc)
{
  uint8_t chunk[64];
  uint32_t crc, pos, left;
  if (offset > end || end - offset < sizeof(*h)) return STORAGE_ERR_RANGE;
  Storage_Status st = Storage_Read(log->map, log->partition, offset, h, sizeof(*h));
  if (st != STORAGE_OK) return st;
  if (h->magic != STORAGE_RECORD_MAGIC || h->format_version != STORAGE_RECORD_FORMAT_V1 ||
      h->header_size != sizeof(*h) || (h->commit_marker & ~STORAGE_COMMIT_MARKER) != 0U)
    return STORAGE_ERR_STATE;
  if (h->payload_length > end - offset - sizeof(*h)) return STORAGE_ERR_RANGE;
  if (StorageRecord_HeaderCrc(h, &crc) != STORAGE_OK || crc != h->header_crc32)
    return STORAGE_ERR_CRC;
  crc = UINT32_MAX;
  pos = offset + sizeof(*h);
  left = h->payload_length;
  while (left) {
    const uint32_t n = left > sizeof(chunk) ? sizeof(chunk) : left;
    st = Storage_Read(log->map, log->partition, pos, chunk, n);
    if (st != STORAGE_OK) return st;
    crc = Storage_Crc32Update(crc, chunk, n);
    pos += n; left -= n;
  }
  if ((crc ^ UINT32_MAX) != h->payload_crc32) return STORAGE_ERR_CRC;
  *loc = (StorageRecordLoc){h->sequence, offset, h->payload_length, 1U};
  return STORAGE_OK;
}
/* Account for acknowledged records as occupied: a reboot must not reset the
 * write cursor/sequence to the last still-pending record. */
static Storage_Status FindLatestLog(const StorageLog *log, uint32_t offset,
    uint32_t size, StorageRecordLoc *latest)
{
  const uint32_t end = offset + size;
  uint8_t found = 0U;
  while (offset <= end && end - offset >= sizeof(StorageRecordHeader)) {
    StorageRecordHeader h;
    StorageRecordLoc loc;
    Storage_Status st = ReadLogRecord(log, offset, end, &h, &loc);
    if (st == STORAGE_OK) {
      if (!found || Storage_SeqIsNewer(loc.sequence, latest->sequence)) *latest = loc;
      found = 1U;
      offset = NorFlash_AlignUp(offset + sizeof(h) + h.payload_length, 4U);
    } else if (st == STORAGE_ERR_STATE || st == STORAGE_ERR_CRC || st == STORAGE_ERR_RANGE) {
      if (h.magic == STORAGE_ERASED_U32) break;
      offset += 4U;
    } else return st;
  }
  return found ? STORAGE_OK : STORAGE_ERR_NOT_FOUND;
}

Storage_Status StorageLog_Init(StorageLog *log, const StoragePartitionMap *map,
                               uint32_t partition, uint32_t region_offset,
                               uint32_t region_size)
{
  uint32_t i;
  uint32_t count;
  uint32_t best_index = 0U;
  uint32_t best_seq = 0U;
  uint8_t have = 0U;
  uint32_t next_seq = 1U;

  if ((log == NULL) || (map == NULL) || (region_size < NOR_FLASH_SECTOR_SIZE) ||
      ((region_size % NOR_FLASH_SECTOR_SIZE) != 0U)) {
    return STORAGE_ERR_PARAM;
  }
  memset(log, 0, sizeof(*log));
  log->map = map;
  log->partition = partition;
  log->region_offset = region_offset;
  log->region_size = region_size;
  count = SectorCount(log);

  for (i = 0U; i < count; ++i) {
    StorageLogSectorHeader header;
    StorageRecordLoc loc;
    Storage_Status st = ReadSectorHeader(log, i, &header);
    if (st != STORAGE_OK) {
      return st;
    }
    if (SectorHeaderValid(&header) == 0) {
      continue;
    }
    if ((have == 0U) ||
        (Storage_SeqIsNewer(header.sector_sequence, best_seq) != 0)) {
      best_index = i;
      best_seq = header.sector_sequence;
      have = 1U;
    }
    st = FindLatestLog(log,
        SectorBase(log, i) + (uint32_t)sizeof(StorageLogSectorHeader),
        NOR_FLASH_SECTOR_SIZE - (uint32_t)sizeof(StorageLogSectorHeader), &loc);
    if (st != STORAGE_OK && st != STORAGE_ERR_NOT_FOUND) return st;
    if (st == STORAGE_OK) {
      if (Storage_SeqIsNewer(loc.sequence + 1U, next_seq) != 0) {
        next_seq = loc.sequence + 1U;
      }
    }
  }

  if (have == 0U) {
    Storage_Status st =
        Storage_EraseSector(map, partition, SectorBase(log, 0U));
    if (st != STORAGE_OK) {
      return st;
    }
    st = CommitSectorHeader(log, 0U, 1U, 1U);
    if (st != STORAGE_OK) {
      return st;
    }
    log->active_sector_index = 0U;
    log->write_offset_in_sector = (uint32_t)sizeof(StorageLogSectorHeader);
    log->next_sequence = 1U;
    return STORAGE_OK;
  }

  log->active_sector_index = best_index;
  {
    StorageRecordLoc loc;
    uint32_t data_off =
        SectorBase(log, best_index) + (uint32_t)sizeof(StorageLogSectorHeader);
    uint32_t data_size =
        NOR_FLASH_SECTOR_SIZE - (uint32_t)sizeof(StorageLogSectorHeader);
    Storage_Status st = FindLatestLog(log, data_off, data_size, &loc);
    if (st != STORAGE_OK && st != STORAGE_ERR_NOT_FOUND) return st;
    if (st == STORAGE_OK) {
      log->write_offset_in_sector =
          (loc.offset - SectorBase(log, best_index)) +
          (uint32_t)sizeof(StorageRecordHeader) + loc.payload_length;
      log->write_offset_in_sector =
          NorFlash_AlignUp(log->write_offset_in_sector, 4U);
      next_seq = loc.sequence + 1U;
    } else {
      log->write_offset_in_sector = (uint32_t)sizeof(StorageLogSectorHeader);
    }
  }
  log->next_sequence = next_seq;
  return STORAGE_OK;
}

static Storage_Status RotateSector(StorageLog *log)
{
  uint32_t count = SectorCount(log);
  uint32_t next = (log->active_sector_index + 1U) % count;
  StorageLogSectorHeader old_hdr;
  Storage_Status st;
  uint32_t erase_count = 1U;
  uint32_t sector_seq = 1U;

  st = ReadSectorHeader(log, log->active_sector_index, &old_hdr);
  if ((st == STORAGE_OK) && (SectorHeaderValid(&old_hdr) != 0)) {
    sector_seq = old_hdr.sector_sequence + 1U;
  }
  st = ReadSectorHeader(log, next, &old_hdr);
  if ((st == STORAGE_OK) && (SectorHeaderValid(&old_hdr) != 0)) {
    erase_count = old_hdr.erase_count + 1U;
  }
  st = Storage_EraseSector(log->map, log->partition, SectorBase(log, next));
  if (st != STORAGE_OK) {
    return st;
  }
  st = CommitSectorHeader(log, next, sector_seq, erase_count);
  if (st != STORAGE_OK) {
    return st;
  }
  log->active_sector_index = next;
  log->write_offset_in_sector = (uint32_t)sizeof(StorageLogSectorHeader);
  return STORAGE_OK;
}

Storage_Status StorageLog_Append(StorageLog *log, const void *payload,
                                 uint32_t payload_length)
{
  Storage_Status st;
  uint32_t need;
  uint32_t abs_off;
  uint32_t sector_end;

  if ((log == NULL) || ((payload == NULL) && (payload_length != 0U))) {
    return STORAGE_ERR_PARAM;
  }
  if (payload_length >
      (UINT32_MAX - (uint32_t)sizeof(StorageRecordHeader))) {
    return STORAGE_ERR_RANGE;
  }
  need = (uint32_t)sizeof(StorageRecordHeader) + payload_length;
  if (need >
      (NOR_FLASH_SECTOR_SIZE - (uint32_t)sizeof(StorageLogSectorHeader))) {
    return STORAGE_ERR_RANGE;
  }
  if ((log->write_offset_in_sector + need) > NOR_FLASH_SECTOR_SIZE) {
    st = RotateSector(log);
    if (st != STORAGE_OK) {
      return st;
    }
  }
  abs_off = SectorBase(log, log->active_sector_index) +
            log->write_offset_in_sector;
  sector_end =
      SectorBase(log, log->active_sector_index) + NOR_FLASH_SECTOR_SIZE;
  st = StorageRecord_WriteAt(log->map, log->partition, abs_off, sector_end,
                             log->next_sequence, payload, payload_length);
  if (st == STORAGE_ERR_NO_SPACE) {
    st = RotateSector(log);
    if (st != STORAGE_OK) {
      return st;
    }
    abs_off = SectorBase(log, log->active_sector_index) +
              log->write_offset_in_sector;
    sector_end =
        SectorBase(log, log->active_sector_index) + NOR_FLASH_SECTOR_SIZE;
    st = StorageRecord_WriteAt(log->map, log->partition, abs_off, sector_end,
                               log->next_sequence, payload, payload_length);
  }
  if (st != STORAGE_OK) {
    return st;
  }
  log->write_offset_in_sector += need;
  log->write_offset_in_sector =
      NorFlash_AlignUp(log->write_offset_in_sector, 4U);
  log->next_sequence += 1U;
  return STORAGE_OK;
}

Storage_Status StorageLog_GetRecent(StorageLog *log, uint32_t max_count,
                                    StorageRecordLoc *out_locs,
                                    uint32_t *out_count)
{
  uint32_t i;
  uint32_t count;
  uint32_t n = 0U;

  if ((log == NULL) || (out_locs == NULL) || (out_count == NULL) ||
      (max_count == 0U)) {
    return STORAGE_ERR_PARAM;
  }
  count = SectorCount(log);
  for (i = 0U; i < count; ++i) {
    StorageLogSectorHeader header;
    uint32_t data_off;
    uint32_t data_size;
    Storage_Status st = ReadSectorHeader(log, i, &header);
    if ((st != STORAGE_OK) || (SectorHeaderValid(&header) == 0)) {
      continue;
    }
    data_off = SectorBase(log, i) + (uint32_t)sizeof(StorageLogSectorHeader);
    data_size =
        NOR_FLASH_SECTOR_SIZE - (uint32_t)sizeof(StorageLogSectorHeader);
    {
      uint32_t cursor = data_off;
      const uint32_t end = data_off + data_size;
      uint32_t guard = 0U;
      while ((cursor <= end) &&
             ((end - cursor) >= (uint32_t)sizeof(StorageRecordHeader))) {
        StorageRecordHeader rh;
        StorageRecordLoc candidate;
        uint32_t next;

        st = Storage_Read(log->map, log->partition, cursor, &rh, sizeof(rh));
        if (st != STORAGE_OK) {
          return st;
        }
        if (rh.magic == STORAGE_ERASED_U32) {
          break;
        }
        if (rh.magic != STORAGE_RECORD_MAGIC) {
          cursor += 4U;
          continue;
        }

        st = StorageRecord_ValidateAt(log->map, log->partition, cursor, end,
                                      &candidate);
        if (st == STORAGE_OK) {
          if (n < max_count) {
            out_locs[n++] = candidate;
          } else {
            uint32_t oldest = 0U;
            uint32_t j;
            for (j = 1U; j < n; ++j) {
              if (Storage_SeqIsNewer(out_locs[oldest].sequence,
                                     out_locs[j].sequence) != 0) {
                oldest = j;
              }
            }
            if (Storage_SeqIsNewer(candidate.sequence,
                                   out_locs[oldest].sequence) != 0) {
              out_locs[oldest] = candidate;
            }
          }
          next = cursor + (uint32_t)sizeof(StorageRecordHeader) +
                 candidate.payload_length;
          next = NorFlash_AlignUp(next, 4U);
          if (next <= cursor) {
            break;
          }
          cursor = next;
        } else if ((st == STORAGE_ERR_CRC) || (st == STORAGE_ERR_STATE) ||
                   (st == STORAGE_ERR_RANGE)) {
          cursor += 4U;
        } else {
          return st;
        }
        if (++guard > (data_size / 4U)) {
          break;
        }
      }
    }
  }
  /* Sort newest first. */
  for (i = 0U; i < n; ++i) {
    uint32_t best = i;
    uint32_t j;
    for (j = i + 1U; j < n; ++j) {
      if (Storage_SeqIsNewer(out_locs[j].sequence,
                             out_locs[best].sequence) != 0) {
        best = j;
      }
    }
    if (best != i) {
      StorageRecordLoc tmp = out_locs[i];
      out_locs[i] = out_locs[best];
      out_locs[best] = tmp;
    }
  }
  *out_count = n;
  return STORAGE_OK;
}

Storage_Status StorageLog_Clear(StorageLog *log)
{
  uint32_t i;
  uint32_t count;
  Storage_Status st;
  StorageLogSectorHeader previous;
  uint32_t generation;

  if (log == NULL) {
    return STORAGE_ERR_PARAM;
  }
  /* Keep the generation monotonic so a receipt held across an explicit clear
   * cannot acknowledge a new record at an identical offset. */
  st = ReadSectorHeader(log, log->active_sector_index, &previous);
  if (st != STORAGE_OK) return st;
  if (!SectorHeaderValid(&previous)) return STORAGE_ERR_STATE;
  generation = previous.sector_sequence + 1U;
  count = SectorCount(log);
  for (i = 0U; i < count; ++i) {
    st = Storage_EraseSector(log->map, log->partition, SectorBase(log, i));
    if (st != STORAGE_OK) {
      return st;
    }
  }
  st = CommitSectorHeader(log, 0U, generation, 1U);
  if (st != STORAGE_OK) {
    return st;
  }
  log->active_sector_index = 0U;
  log->write_offset_in_sector = (uint32_t)sizeof(StorageLogSectorHeader);
  log->next_sequence = 1U;
  return STORAGE_OK;
}

static Storage_Status NextSector(StorageLog *log, StorageLogCursor *cursor)
{
  cursor->sector = (cursor->sector + 1U) % SectorCount(log);
  cursor->offset = sizeof(StorageLogSectorHeader);
  if (--cursor->remaining == 0U) {
    cursor->started = 0U;
    return STORAGE_ERR_NOT_FOUND;
  }
  return STORAGE_ERR_BUSY;
}
Storage_Status StorageLog_NextPending(StorageLog *log, StorageLogCursor *cursor,
    StorageLogReceipt *receipt, void *payload, uint32_t capacity, uint32_t *length)
{
  StorageLogSectorHeader sector;
  Storage_Status st;
  if (!log || !log->map || !cursor || !receipt || !payload || !length || !SectorCount(log))
    return STORAGE_ERR_PARAM;
  *length = 0U;
  memset(receipt, 0, sizeof(*receipt));
  if (!cursor->started) {
    cursor->sector = (log->active_sector_index + 1U) % SectorCount(log);
    cursor->offset = sizeof(StorageLogSectorHeader);
    cursor->remaining = SectorCount(log);
    cursor->started = 1U;
  }
  if (cursor->sector >= SectorCount(log) || !cursor->remaining ||
      cursor->remaining > SectorCount(log) || cursor->offset < sizeof(sector) ||
      cursor->offset > NOR_FLASH_SECTOR_SIZE) return STORAGE_ERR_PARAM;
  st = ReadSectorHeader(log, cursor->sector, &sector);
  if (st != STORAGE_OK) return st;
  if (!SectorHeaderValid(&sector)) return NextSector(log, cursor);
  const uint32_t base = SectorBase(log, cursor->sector);
  const uint32_t end = base + NOR_FLASH_SECTOR_SIZE;
  /* Eight candidates at most, even for corrupt bytes or already-ACKed logs.
   * Max additional payload validation per candidate is one sector. */
  for (uint32_t budget = 0U; budget < 8U; ++budget) {
    StorageRecordHeader h;
    StorageRecordLoc loc;
    if (NOR_FLASH_SECTOR_SIZE - cursor->offset < sizeof(h)) return NextSector(log, cursor);
    st = ReadLogRecord(log, base + cursor->offset, end, &h, &loc);
    if (st == STORAGE_OK) {
      const uint32_t next = NorFlash_AlignUp(cursor->offset + sizeof(h) + h.payload_length, 4U);
      if (h.commit_marker == 0U) { cursor->offset = next; continue; }
      if (h.payload_length > capacity) return STORAGE_ERR_RANGE;
      st = Storage_Read(log->map, log->partition, loc.offset + sizeof(h), payload, h.payload_length);
      if (st != STORAGE_OK) return st;
      if (Storage_Crc32(payload, h.payload_length) != h.payload_crc32) return STORAGE_ERR_CRC;
      receipt->record = loc;
      receipt->sector_sequence = sector.sector_sequence;
      receipt->payload_crc32 = h.payload_crc32;
      *length = h.payload_length;
      cursor->offset = next;
      return STORAGE_OK;
    }
    if (st != STORAGE_ERR_STATE && st != STORAGE_ERR_RANGE && st != STORAGE_ERR_CRC) return st;
    if (h.magic == STORAGE_ERASED_U32) return NextSector(log, cursor);
    cursor->offset += 4U;
    if (st == STORAGE_ERR_CRC) return st; /* Observable, retry resumes scanning. */
  }
  return STORAGE_ERR_BUSY;
}
Storage_Status StorageLog_Acknowledge(StorageLog *log, const StorageLogReceipt *receipt)
{
  StorageLogSectorHeader sector;
  StorageRecordHeader h;
  StorageRecordLoc loc;
  uint32_t ack = 0U, verify;
  Storage_Status st;
  if (!log || !log->map || !receipt || !receipt->record.valid) return STORAGE_ERR_PARAM;
  const uint32_t offset = receipt->record.offset;
  if (offset < log->region_offset || offset - log->region_offset >= log->region_size)
    return STORAGE_ERR_RANGE;
  const uint32_t index = (offset - log->region_offset) / NOR_FLASH_SECTOR_SIZE;
  if (offset - SectorBase(log, index) < sizeof(sector)) return STORAGE_ERR_RANGE;
  st = ReadSectorHeader(log, index, &sector);
  if (st != STORAGE_OK) return st;
  if (!SectorHeaderValid(&sector) || sector.sector_sequence != receipt->sector_sequence)
    return STORAGE_ERR_NOT_FOUND;
  st = ReadLogRecord(log, offset, SectorBase(log, index) + NOR_FLASH_SECTOR_SIZE, &h, &loc);
  if (st != STORAGE_OK) return st;
  if (loc.sequence != receipt->record.sequence || loc.payload_length != receipt->record.payload_length ||
      h.payload_crc32 != receipt->payload_crc32) return STORAGE_ERR_NOT_FOUND;
  if (h.commit_marker == 0U) return STORAGE_OK;
  const uint32_t marker = offset + offsetof(StorageRecordHeader, commit_marker);
  st = Storage_Write(log->map, log->partition, marker, &ack, sizeof(ack));
  if (st != STORAGE_OK) return st;
  st = Storage_Read(log->map, log->partition, marker, &verify, sizeof(verify));
  if (st != STORAGE_OK) return st;
  return verify == 0U ? STORAGE_OK : STORAGE_ERR_STATE;
}
