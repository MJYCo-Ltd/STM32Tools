#include "Flash/storage_record.h"

#include "Flash/nor_flash.h"
#include "Flash/storage_commit.h"
#include "Flash/storage_crc.h"

#include <stddef.h>
#include <string.h>

int Storage_SeqIsNewer(uint32_t a, uint32_t b)
{
  return ((int32_t)(a - b) > 0) ? 1 : 0;
}

Storage_Status StorageRecord_HeaderCrc(const StorageRecordHeader *header,
                                       uint32_t *crc_out)
{
  return Storage_ComputeCrcExcludingCommit(
      header, sizeof(*header), offsetof(StorageRecordHeader, header_crc32),
      crc_out);
}

static Storage_Status IsErasedRange(const StoragePartitionMap *map,
                                    uint32_t partition, uint32_t offset,
                                    uint32_t length)
{
  uint8_t chunk[64];
  uint32_t remaining = length;
  uint32_t pos = offset;

  while (remaining > 0U) {
    uint32_t n = (remaining > sizeof(chunk)) ? (uint32_t)sizeof(chunk) : remaining;
    uint32_t i;
    Storage_Status st = Storage_Read(map, partition, pos, chunk, n);
    if (st != STORAGE_OK) {
      return st;
    }
    for (i = 0U; i < n; ++i) {
      if (chunk[i] != 0xFFU) {
        return STORAGE_ERR_STATE;
      }
    }
    pos += n;
    remaining -= n;
  }
  return STORAGE_OK;
}

static Storage_Status ValidateHeader(const StorageRecordHeader *header,
                                     uint32_t region_end_offset,
                                     uint32_t record_offset)
{
  uint32_t total;
  uint32_t header_crc;

  if ((header->magic != STORAGE_RECORD_MAGIC) ||
      (header->format_version != STORAGE_RECORD_FORMAT_V1) ||
      (header->header_size != (uint16_t)sizeof(StorageRecordHeader))) {
    return STORAGE_ERR_STATE;
  }
  if (header->commit_marker != STORAGE_COMMIT_MARKER) {
    return STORAGE_ERR_STATE;
  }
  if ((StorageRecord_HeaderCrc(header, &header_crc) != STORAGE_OK) ||
      (header_crc != header->header_crc32)) {
    return STORAGE_ERR_CRC;
  }
  if ((header->payload_length >
       (UINT32_MAX - (uint32_t)sizeof(StorageRecordHeader))) ||
      ((record_offset + (uint32_t)sizeof(StorageRecordHeader)) < record_offset)) {
    return STORAGE_ERR_RANGE;
  }
  total = record_offset + (uint32_t)sizeof(StorageRecordHeader) +
          header->payload_length;
  if ((total < record_offset) || (total > region_end_offset)) {
    return STORAGE_ERR_RANGE;
  }
  return STORAGE_OK;
}

Storage_Status StorageRecord_WriteAt(
    const StoragePartitionMap *map, uint32_t partition, uint32_t offset,
    uint32_t max_end, uint32_t sequence, const void *payload,
    uint32_t payload_length)
{
  StorageRecordHeader header;
  StorageRecordHeader verify;
  uint32_t need;
  Storage_Status st;
  uint32_t commit = STORAGE_COMMIT_MARKER;

  if ((map == NULL) || ((payload == NULL) && (payload_length != 0U))) {
    return STORAGE_ERR_PARAM;
  }
  if (payload_length >
      (UINT32_MAX - (uint32_t)sizeof(StorageRecordHeader))) {
    return STORAGE_ERR_RANGE;
  }
  need = (uint32_t)sizeof(StorageRecordHeader) + payload_length;
  if ((offset + need) < offset || ((offset + need) > max_end)) {
    return STORAGE_ERR_NO_SPACE;
  }
  st = IsErasedRange(map, partition, offset, need);
  if (st != STORAGE_OK) {
    return (st == STORAGE_ERR_STATE) ? STORAGE_ERR_NO_SPACE : st;
  }

  memset(&header, 0, sizeof(header));
  header.magic = STORAGE_RECORD_MAGIC;
  header.format_version = STORAGE_RECORD_FORMAT_V1;
  header.header_size = (uint16_t)sizeof(StorageRecordHeader);
  header.sequence = sequence;
  header.payload_length = payload_length;
  header.payload_crc32 =
      (payload_length == 0U) ? 0U : Storage_Crc32(payload, payload_length);
  st = Storage_PrepareCommitObject(
      &header, sizeof(header), offsetof(StorageRecordHeader, header_crc32));
  if (st != STORAGE_OK) {
    return st;
  }

  st = Storage_Write(map, partition, offset, &header,
                     (uint32_t)sizeof(header) - sizeof(uint32_t));
  if (st != STORAGE_OK) {
    return st;
  }
  if (payload_length > 0U) {
    st = Storage_Write(map, partition,
                       offset + (uint32_t)sizeof(StorageRecordHeader), payload,
                       payload_length);
    if (st != STORAGE_OK) {
      return st;
    }
  }

  st = Storage_Read(map, partition, offset, &verify,
                    (uint32_t)sizeof(verify) - sizeof(uint32_t));
  if (st != STORAGE_OK) {
    return st;
  }
  if (memcmp(&verify, &header, sizeof(header) - sizeof(uint32_t)) != 0) {
    return STORAGE_ERR_CRC;
  }

  return Storage_Write(map, partition,
                       offset + (uint32_t)offsetof(StorageRecordHeader,
                                                   commit_marker),
                       &commit, sizeof(commit));
}

Storage_Status StorageRecord_Append(
    const StoragePartitionMap *map, uint32_t partition, uint32_t region_offset,
    uint32_t region_size, uint32_t sequence, const void *payload,
    uint32_t payload_length, uint32_t *written_offset)
{
  StorageRecordLoc latest;
  uint32_t cursor;
  uint32_t region_end;
  Storage_Status st;

  if ((map == NULL) || ((payload == NULL) && (payload_length != 0U))) {
    return STORAGE_ERR_PARAM;
  }
  if ((region_size == 0U) ||
      (payload_length > (UINT32_MAX - (uint32_t)sizeof(StorageRecordHeader)))) {
    return STORAGE_ERR_RANGE;
  }
  region_end = region_offset + region_size;
  if (region_end < region_offset) {
    return STORAGE_ERR_RANGE;
  }

  memset(&latest, 0, sizeof(latest));
  st = StorageRecord_FindLatest(map, partition, region_offset, region_size,
                                &latest, NULL, 0U);
  if ((st != STORAGE_OK) && (st != STORAGE_ERR_NOT_FOUND)) {
    return st;
  }
  if (st == STORAGE_OK) {
    cursor = latest.offset + (uint32_t)sizeof(StorageRecordHeader) +
             latest.payload_length;
    cursor = NorFlash_AlignUp(cursor, 4U);
  } else {
    cursor = region_offset;
  }

  st = StorageRecord_WriteAt(map, partition, cursor, region_end, sequence,
                             payload, payload_length);
  if (st != STORAGE_OK) {
    return st;
  }
  if (written_offset != NULL) {
    *written_offset = cursor;
  }
  return STORAGE_OK;
}

Storage_Status StorageRecord_FindLatest(
    const StoragePartitionMap *map, uint32_t partition, uint32_t region_offset,
    uint32_t region_size, StorageRecordLoc *out_loc, void *payload_buf,
    uint32_t payload_buf_size)
{
  uint32_t cursor;
  uint32_t region_end;
  uint32_t steps = 0U;
  const uint32_t max_steps = (region_size / 4U) + 1U;
  StorageRecordLoc best;
  Storage_Status found = STORAGE_ERR_NOT_FOUND;

  if ((map == NULL) || (out_loc == NULL) || (region_size == 0U)) {
    return STORAGE_ERR_PARAM;
  }
  region_end = region_offset + region_size;
  if (region_end < region_offset) {
    return STORAGE_ERR_RANGE;
  }

  memset(&best, 0, sizeof(best));
  cursor = region_offset;
  while ((cursor <= region_end) &&
         ((region_end - cursor) >= (uint32_t)sizeof(StorageRecordHeader))) {
    StorageRecordHeader header;
    Storage_Status st;
    uint32_t next;
    uint8_t payload_ok = 1U;

    if (++steps > max_steps) {
      break;
    }
    st = Storage_Read(map, partition, cursor, &header, sizeof(header));
    if (st != STORAGE_OK) {
      return st;
    }
    if (header.magic == STORAGE_ERASED_U32) {
      break;
    }
    if (header.magic != STORAGE_RECORD_MAGIC) {
      cursor = (cursor + 4U);
      continue;
    }
    st = ValidateHeader(&header, region_end, cursor);
    if (st != STORAGE_OK) {
      cursor = (cursor + 4U);
      continue;
    }
    if ((payload_buf != NULL) && (header.payload_length > 0U)) {
      if (header.payload_length > payload_buf_size) {
        payload_ok = 0U;
      } else {
        st = Storage_Read(map, partition,
                          cursor + (uint32_t)sizeof(StorageRecordHeader),
                          payload_buf, header.payload_length);
        if (st != STORAGE_OK) {
          return st;
        }
        if (Storage_Crc32(payload_buf, header.payload_length) !=
            header.payload_crc32) {
          payload_ok = 0U;
        }
      }
    } else if (header.payload_length > 0U) {
      uint8_t tmp[64];
      uint32_t left = header.payload_length;
      uint32_t pos = cursor + (uint32_t)sizeof(StorageRecordHeader);
      uint32_t crc = 0xFFFFFFFFUL;
      while (left > 0U) {
        uint32_t n = (left > sizeof(tmp)) ? (uint32_t)sizeof(tmp) : left;
        st = Storage_Read(map, partition, pos, tmp, n);
        if (st != STORAGE_OK) {
          return st;
        }
        crc = Storage_Crc32Update(crc, tmp, n);
        pos += n;
        left -= n;
      }
      if ((crc ^ 0xFFFFFFFFUL) != header.payload_crc32) {
        payload_ok = 0U;
      }
    }
    if (payload_ok != 0U) {
      if ((found != STORAGE_OK) ||
          (Storage_SeqIsNewer(header.sequence, best.sequence) != 0)) {
        best.sequence = header.sequence;
        best.offset = cursor;
        best.payload_length = header.payload_length;
        best.valid = 1U;
        found = STORAGE_OK;
        if ((payload_buf != NULL) && (header.payload_length > 0U) &&
            (header.payload_length <= payload_buf_size)) {
          /* payload_buf already filled when size allowed */
        }
      }
    }
    next = cursor + (uint32_t)sizeof(StorageRecordHeader) +
           header.payload_length;
    next = NorFlash_AlignUp(next, 4U);
    if (next <= cursor) {
      break;
    }
    cursor = next;
  }
  *out_loc = best;
  return found;
}

Storage_Status StorageRecord_ReadPayload(
    const StoragePartitionMap *map, uint32_t partition, uint32_t record_offset,
    void *payload_buf, uint32_t payload_buf_size, uint32_t *payload_length)
{
  StorageRecordHeader header;
  Storage_Status st;

  if ((map == NULL) || (payload_buf == NULL)) {
    return STORAGE_ERR_PARAM;
  }
  st = Storage_Read(map, partition, record_offset, &header, sizeof(header));
  if (st != STORAGE_OK) {
    return st;
  }
  st = ValidateHeader(&header, map->parts[partition].size, record_offset);
  if (st != STORAGE_OK) {
    return st;
  }
  if (header.payload_length > payload_buf_size) {
    return STORAGE_ERR_RANGE;
  }
  if (header.payload_length > 0U) {
    st = Storage_Read(map, partition,
                      record_offset + (uint32_t)sizeof(StorageRecordHeader),
                      payload_buf, header.payload_length);
    if (st != STORAGE_OK) {
      return st;
    }
    if (Storage_Crc32(payload_buf, header.payload_length) !=
        header.payload_crc32) {
      return STORAGE_ERR_CRC;
    }
  }
  if (payload_length != NULL) {
    *payload_length = header.payload_length;
  }
  return STORAGE_OK;
}
