#include "Flash/storage_bank.h"

#include "Flash/nor_flash.h"
#include "Flash/storage_record.h"

#include <stddef.h>

static int RegionsOverlap(uint32_t a_offset, uint32_t a_size,
                          uint32_t b_offset, uint32_t b_size)
{
  return (a_offset < (b_offset + b_size)) &&
         (b_offset < (a_offset + a_size));
}

static Storage_Status ValidateRegion(uint32_t partition_size, uint32_t offset,
                                     uint32_t size)
{
  if ((size == 0U) || ((offset % NOR_FLASH_SECTOR_SIZE) != 0U) ||
      ((size % NOR_FLASH_SECTOR_SIZE) != 0U)) {
    return STORAGE_ERR_PARAM;
  }
  return (NorFlash_CheckRange(partition_size, offset, size) == NOR_FLASH_OK)
             ? STORAGE_OK
             : STORAGE_ERR_RANGE;
}

Storage_Status StorageBank_EraseRegion(const StoragePartitionMap *map,
                                       uint32_t partition,
                                       uint32_t region_offset,
                                       uint32_t region_size)
{
  const StoragePartDesc *part;
  uint32_t offset;
  Storage_Status st;

  if ((map == NULL) || (map->parts == NULL) ||
      (partition >= map->part_count)) {
    return STORAGE_ERR_PARAM;
  }
  part = &map->parts[partition];
  if (part->readonly != 0U) {
    return STORAGE_ERR_READONLY;
  }
  st = ValidateRegion(part->size, region_offset, region_size);
  if (st != STORAGE_OK) {
    return st;
  }
  for (offset = 0U; offset < region_size; offset += NOR_FLASH_SECTOR_SIZE) {
    st = Storage_EraseSector(map, partition, region_offset + offset);
    if (st != STORAGE_OK) {
      return st;
    }
  }
  return STORAGE_OK;
}

Storage_Status StorageBank_Switch(const StoragePartitionMap *map,
                                  uint32_t partition, uint32_t dest_offset,
                                  uint32_t dest_size, uint32_t old_offset,
                                  uint32_t old_size, uint32_t sequence,
                                  const void *payload, uint32_t payload_length,
                                  uint32_t extra_erase_offset,
                                  uint32_t extra_erase_size)
{
  const StoragePartDesc *part;
  uint32_t record_size;
  Storage_Status st;

  if ((map == NULL) || (map->parts == NULL) ||
      (partition >= map->part_count) ||
      ((payload == NULL) && (payload_length != 0U)) ||
      (payload_length >
       (UINT32_MAX - (uint32_t)sizeof(StorageRecordHeader)))) {
    return STORAGE_ERR_PARAM;
  }
  part = &map->parts[partition];
  if (part->readonly != 0U) {
    return STORAGE_ERR_READONLY;
  }
  record_size = (uint32_t)sizeof(StorageRecordHeader) + payload_length;
  if ((ValidateRegion(part->size, dest_offset, dest_size) != STORAGE_OK) ||
      (ValidateRegion(part->size, old_offset, old_size) != STORAGE_OK) ||
      (record_size > dest_size)) {
    return STORAGE_ERR_RANGE;
  }
  if (RegionsOverlap(dest_offset, dest_size, old_offset, old_size) != 0) {
    return STORAGE_ERR_RANGE;
  }
  if (extra_erase_size > 0U) {
    if ((ValidateRegion(part->size, extra_erase_offset, extra_erase_size) !=
         STORAGE_OK) ||
        (RegionsOverlap(dest_offset, dest_size, extra_erase_offset,
                        extra_erase_size) != 0) ||
        (RegionsOverlap(old_offset, old_size, extra_erase_offset,
                        extra_erase_size) != 0)) {
      return STORAGE_ERR_RANGE;
    }
  }

  st = StorageBank_EraseRegion(map, partition, dest_offset, dest_size);
  if (st != STORAGE_OK) {
    return st;
  }
  st = StorageRecord_WriteAt(map, partition, dest_offset,
                             dest_offset + dest_size, sequence, payload,
                             payload_length);
  if (st != STORAGE_OK) {
    return st;
  }
  st = StorageBank_EraseRegion(map, partition, old_offset, old_size);
  if (st != STORAGE_OK) {
    return st;
  }
  if (extra_erase_size > 0U) {
    st = StorageBank_EraseRegion(map, partition, extra_erase_offset,
                                 extra_erase_size);
  }
  return st;
}
