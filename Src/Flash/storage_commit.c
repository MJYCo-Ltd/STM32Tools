#include "Flash/storage_commit.h"

#include "Flash/storage_crc.h"

#include <string.h>

Storage_Status Storage_ComputeCrcExcludingCommit(
    const void *object, size_t total_size, size_t crc_field_offset,
    uint32_t *crc_out)
{
  uint8_t tmp[256];

  if ((object == NULL) || (crc_out == NULL) ||
      (total_size < (sizeof(uint32_t) * 2U)) ||
      (total_size > sizeof(tmp)) ||
      (crc_field_offset > (total_size - (sizeof(uint32_t) * 2U)))) {
    return STORAGE_ERR_PARAM;
  }
  memcpy(tmp, object, total_size);
  memset(tmp + crc_field_offset, 0, sizeof(uint32_t));
  memset(tmp + total_size - sizeof(uint32_t), 0xFF, sizeof(uint32_t));
  *crc_out = Storage_Crc32(tmp, total_size - sizeof(uint32_t));
  return STORAGE_OK;
}

Storage_Status Storage_PrepareCommitObject(void *object, size_t total_size,
                                           size_t crc_field_offset)
{
  uint8_t *bytes;
  uint32_t crc;

  if ((object == NULL) || (total_size < (sizeof(uint32_t) * 2U)) ||
      (total_size > 256U) ||
      (crc_field_offset > (total_size - (sizeof(uint32_t) * 2U))) ||
      ((crc_field_offset % sizeof(uint32_t)) != 0U)) {
    return STORAGE_ERR_PARAM;
  }

  bytes = (uint8_t *)object;
  if (Storage_ComputeCrcExcludingCommit(object, total_size, crc_field_offset,
                                        &crc) != STORAGE_OK) {
    return STORAGE_ERR_PARAM;
  }
  memcpy(bytes + crc_field_offset, &crc, sizeof(crc));
  memset(bytes + total_size - sizeof(uint32_t), 0xFF, sizeof(uint32_t));
  return STORAGE_OK;
}

Storage_Status Storage_WriteCommitted(const StoragePartitionMap *map,
                                      uint32_t partition, uint32_t offset,
                                      const void *object, size_t total_size)
{
  const uint8_t *bytes;
  uint8_t verify[256];
  uint32_t commit = STORAGE_COMMIT_MARKER;
  Storage_Status st;
  size_t body;

  if ((map == NULL) || (object == NULL) ||
      (total_size < (sizeof(uint32_t) * 2U)) || (total_size > sizeof(verify))) {
    return STORAGE_ERR_PARAM;
  }

  bytes = (const uint8_t *)object;
  body = total_size - sizeof(uint32_t);
  st = Storage_Write(map, partition, offset, bytes, (uint32_t)body);
  if (st != STORAGE_OK) {
    return st;
  }
  st = Storage_Read(map, partition, offset, verify, (uint32_t)body);
  if (st != STORAGE_OK) {
    return st;
  }
  if (memcmp(verify, bytes, body) != 0) {
    return STORAGE_ERR_CRC;
  }
  return Storage_Write(map, partition, offset + (uint32_t)body, &commit,
                       sizeof(commit));
}

Storage_Status Storage_CommitObject(const StoragePartitionMap *map,
                                    uint32_t partition, uint32_t offset,
                                    void *object, size_t total_size,
                                    size_t crc_field_offset)
{
  uint32_t commit = STORAGE_COMMIT_MARKER;
  Storage_Status st =
      Storage_PrepareCommitObject(object, total_size, crc_field_offset);
  if (st != STORAGE_OK) {
    return st;
  }
  st = Storage_WriteCommitted(map, partition, offset, object, total_size);
  if (st != STORAGE_OK) {
    return st;
  }
  memcpy((uint8_t *)object + total_size - sizeof(uint32_t), &commit,
         sizeof(commit));
  return STORAGE_OK;
}
