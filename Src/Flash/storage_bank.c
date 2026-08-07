#include "Flash/storage_bank.h"

#include "Flash/nor_flash.h"
#include "Flash/storage_record.h"

#include <stddef.h>

Storage_Status StorageBank_EraseRegion(const StoragePartitionMap *map,
                                       uint32_t partition,
                                       uint32_t region_offset,
                                       uint32_t region_size)
{
  uint32_t offset;
  Storage_Status st;

  if ((map == NULL) || (region_size == 0U) ||
      ((region_offset % NOR_FLASH_SECTOR_SIZE) != 0U) ||
      ((region_size % NOR_FLASH_SECTOR_SIZE) != 0U)) {
    return STORAGE_ERR_PARAM;
  }
  if ((region_offset + region_size) < region_offset) {
    return STORAGE_ERR_RANGE;
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
  Storage_Status st;

  if ((map == NULL) || ((payload == NULL) && (payload_length != 0U)) ||
      (dest_size < NOR_FLASH_SECTOR_SIZE)) {
    return STORAGE_ERR_PARAM;
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
