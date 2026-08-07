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

static uint32_t SectorHeaderCrc(const StorageLogSectorHeader *header)
{
  if (header == NULL) {
    return 0U;
  }
  return Storage_CrcExcludingCommit(
      header, sizeof(*header),
      offsetof(StorageLogSectorHeader, header_crc32));
}

static Storage_Status ReadSectorHeader(const StorageLog *log, uint32_t index,
                                       StorageLogSectorHeader *header)
{
  return Storage_Read(log->map, log->partition, SectorBase(log, index), header,
                      sizeof(*header));
}

static int SectorHeaderValid(const StorageLogSectorHeader *header)
{
  if ((header->magic != STORAGE_LOG_SECTOR_MAGIC) ||
      (header->commit_marker != STORAGE_COMMIT_MARKER)) {
    return 0;
  }
  return (SectorHeaderCrc(header) == header->header_crc32) ? 1 : 0;
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
    st = StorageRecord_FindLatest(
        map, partition,
        SectorBase(log, i) + (uint32_t)sizeof(StorageLogSectorHeader),
        NOR_FLASH_SECTOR_SIZE - (uint32_t)sizeof(StorageLogSectorHeader), &loc,
        NULL, 0U);
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
    Storage_Status st = StorageRecord_FindLatest(
        map, partition, data_off, data_size, &loc, NULL, 0U);
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
  need = (uint32_t)sizeof(StorageRecordHeader) + payload_length;
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
  StorageRecordLoc all[64];
  uint32_t all_n = 0U;

  if ((log == NULL) || (out_locs == NULL) || (out_count == NULL) ||
      (max_count == 0U)) {
    return STORAGE_ERR_PARAM;
  }
  count = SectorCount(log);
  for (i = 0U; i < count; ++i) {
    StorageLogSectorHeader header;
    StorageRecordLoc loc;
    uint32_t data_off;
    uint32_t data_size;
    Storage_Status st = ReadSectorHeader(log, i, &header);
    if ((st != STORAGE_OK) || (SectorHeaderValid(&header) == 0)) {
      continue;
    }
    data_off = SectorBase(log, i) + (uint32_t)sizeof(StorageLogSectorHeader);
    data_size =
        NOR_FLASH_SECTOR_SIZE - (uint32_t)sizeof(StorageLogSectorHeader);
    /* Collect by repeatedly finding; for host tests keep simple: one latest
       per sector is not enough — scan with FindLatest only gets one.
       Use sequential walk via FindLatest on shrinking regions is heavy;
       call FindLatest then walk is incomplete. For v1 collect latest from
       each sector then sort by sequence. */
    st = StorageRecord_FindLatest(log->map, log->partition, data_off, data_size,
                                  &loc, NULL, 0U);
    if (st == STORAGE_OK) {
      /* Walk all records in sector by re-scanning from data_off */
      uint32_t cursor = data_off;
      uint32_t end = data_off + data_size;
      uint32_t guard = 0U;
      while ((cursor + sizeof(StorageRecordHeader)) <= end) {
        StorageRecordHeader rh;
        st = Storage_Read(log->map, log->partition, cursor, &rh, sizeof(rh));
        if (st != STORAGE_OK) {
          return st;
        }
        if (rh.magic == STORAGE_ERASED_U32) {
          break;
        }
        if ((rh.magic == STORAGE_RECORD_MAGIC) &&
            (rh.commit_marker == STORAGE_COMMIT_MARKER) &&
            (StorageRecord_HeaderCrc(&rh) == rh.header_crc32) &&
            (all_n < (sizeof(all) / sizeof(all[0])))) {
          all[all_n].sequence = rh.sequence;
          all[all_n].offset = cursor;
          all[all_n].payload_length = rh.payload_length;
          all[all_n].valid = 1U;
          ++all_n;
        }
        if (rh.magic == STORAGE_RECORD_MAGIC) {
          uint32_t next =
              cursor + (uint32_t)sizeof(StorageRecordHeader) + rh.payload_length;
          next = NorFlash_AlignUp(next, 4U);
          if (next <= cursor) {
            break;
          }
          cursor = next;
        } else {
          cursor += 4U;
        }
        if (++guard > (data_size / 4U)) {
          break;
        }
      }
    }
  }
  /* Pick max_count newest */
  while (n < max_count) {
    uint32_t best = 0U;
    uint8_t have = 0U;
    for (i = 0U; i < all_n; ++i) {
      if (all[i].valid == 0U) {
        continue;
      }
      if ((have == 0U) ||
          (Storage_SeqIsNewer(all[i].sequence, all[best].sequence) != 0)) {
        best = i;
        have = 1U;
      }
    }
    if (have == 0U) {
      break;
    }
    out_locs[n] = all[best];
    all[best].valid = 0U;
    ++n;
  }
  *out_count = n;
  return STORAGE_OK;
}

Storage_Status StorageLog_Clear(StorageLog *log)
{
  uint32_t i;
  uint32_t count;
  Storage_Status st;

  if (log == NULL) {
    return STORAGE_ERR_PARAM;
  }
  count = SectorCount(log);
  for (i = 0U; i < count; ++i) {
    st = Storage_EraseSector(log->map, log->partition, SectorBase(log, i));
    if (st != STORAGE_OK) {
      return st;
    }
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
