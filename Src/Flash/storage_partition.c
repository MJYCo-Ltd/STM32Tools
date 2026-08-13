#include "Flash/storage_partition.h"

#include <stddef.h>

static Storage_Status MapPart(const StoragePartitionMap *map,
                              uint32_t partition, uint32_t offset,
                              uint32_t length, uint32_t *phys_out,
                              uint8_t for_write)
{
  const StoragePartDesc *part;
  uint32_t phys;

  if ((map == NULL) || (map->backend == NULL) || (map->parts == NULL) ||
      (partition >= map->part_count) || (length == 0U)) {
    return STORAGE_ERR_PARAM;
  }
  part = &map->parts[partition];
  if ((for_write != 0U) && (part->readonly != 0U)) {
    return STORAGE_ERR_READONLY;
  }
  if (NorFlash_CheckRange(part->size, offset, length) != NOR_FLASH_OK) {
    return STORAGE_ERR_RANGE;
  }
  if (NorFlash_CheckRange(map->media_size, part->base, part->size) !=
      NOR_FLASH_OK) {
    return STORAGE_ERR_RANGE;
  }
  /* offset + base cannot overflow: both already bounded by media_size. */
  phys = part->base + offset;
  if (NorFlash_CheckRange(map->media_size, phys, length) != NOR_FLASH_OK) {
    return STORAGE_ERR_RANGE;
  }
  if (phys_out != NULL) {
    *phys_out = phys;
  }
  return STORAGE_OK;
}

Storage_Status StoragePartition_ValidateTable(const StoragePartDesc *parts,
                                              uint32_t part_count,
                                              uint32_t media_size)
{
  uint32_t i;
  uint32_t j;

  if ((parts == NULL) || (part_count == 0U) || (media_size == 0U)) {
    return STORAGE_ERR_PARAM;
  }
  for (i = 0U; i < part_count; ++i) {
    if ((parts[i].size == 0U) ||
        ((parts[i].base % NOR_FLASH_SECTOR_SIZE) != 0U) ||
        ((parts[i].size % NOR_FLASH_SECTOR_SIZE) != 0U)) {
      return STORAGE_ERR_RANGE;
    }
    if (NorFlash_CheckRange(media_size, parts[i].base, parts[i].size) !=
        NOR_FLASH_OK) {
      return STORAGE_ERR_RANGE;
    }
    for (j = i + 1U; j < part_count; ++j) {
      const uint32_t a0 = parts[i].base;
      const uint32_t a1 = parts[i].base + parts[i].size;
      const uint32_t b0 = parts[j].base;
      const uint32_t b1 = parts[j].base + parts[j].size;
      if ((a0 < b1) && (b0 < a1)) {
        return STORAGE_ERR_RANGE;
      }
    }
  }
  return STORAGE_OK;
}

Storage_Status StoragePartition_Init(StoragePartitionMap *map,
                                     const StorageBackend *backend,
                                     const StoragePartDesc *parts,
                                     uint32_t part_count,
                                     uint32_t media_size)
{
  Storage_Status st;

  if ((map == NULL) || (backend == NULL) || (parts == NULL) ||
      (backend->read == NULL) || (backend->write == NULL) ||
      (backend->erase_sector == NULL) ||
      ((backend->lock == NULL) != (backend->unlock == NULL))) {
    return STORAGE_ERR_PARAM;
  }
  st = StoragePartition_ValidateTable(parts, part_count, media_size);
  if (st != STORAGE_OK) {
    return st;
  }
  map->backend = backend;
  map->parts = parts;
  map->part_count = part_count;
  map->media_size = media_size;
  return STORAGE_OK;
}

Storage_Status Storage_PhysFromPart(const StoragePartitionMap *map,
                                    uint32_t partition, uint32_t offset,
                                    uint32_t length, uint32_t *phys_out)
{
  return MapPart(map, partition, offset, length, phys_out, 0U);
}

Storage_Status Storage_Read(const StoragePartitionMap *map, uint32_t partition,
                            uint32_t offset, void *buffer, uint32_t length)
{
  uint32_t phys = 0U;
  Storage_Status st;

  if (buffer == NULL) {
    return STORAGE_ERR_PARAM;
  }
  st = MapPart(map, partition, offset, length, &phys, 0U);
  if (st != STORAGE_OK) {
    return st;
  }
  StorageBackend_Lock(map->backend);
  st = map->backend->read(map->backend->ctx, phys, buffer, length);
  StorageBackend_Unlock(map->backend);
  return st;
}

Storage_Status Storage_Write(const StoragePartitionMap *map, uint32_t partition,
                             uint32_t offset, const void *buffer,
                             uint32_t length)
{
  uint32_t phys = 0U;
  Storage_Status st;

  if (buffer == NULL) {
    return STORAGE_ERR_PARAM;
  }
  st = MapPart(map, partition, offset, length, &phys, 1U);
  if (st != STORAGE_OK) {
    return st;
  }
  StorageBackend_Lock(map->backend);
  st = StorageBackend_CheckVoltage(map->backend);
  if (st == STORAGE_OK) {
    st = map->backend->write(map->backend->ctx, phys, buffer, length);
  }
  StorageBackend_Unlock(map->backend);
  return st;
}

Storage_Status Storage_EraseSector(const StoragePartitionMap *map,
                                   uint32_t partition, uint32_t offset)
{
  uint32_t phys = 0U;
  Storage_Status st;

  if ((offset % NOR_FLASH_SECTOR_SIZE) != 0U) {
    return STORAGE_ERR_RANGE;
  }
  st = MapPart(map, partition, offset, NOR_FLASH_SECTOR_SIZE, &phys, 1U);
  if (st != STORAGE_OK) {
    return st;
  }
  StorageBackend_Lock(map->backend);
  st = StorageBackend_CheckVoltage(map->backend);
  if (st == STORAGE_OK) {
    st = map->backend->erase_sector(map->backend->ctx, phys);
  }
  StorageBackend_Unlock(map->backend);
  return st;
}

Storage_Status Storage_EraseBlock64(const StoragePartitionMap *map,
                                    uint32_t partition, uint32_t offset)
{
  uint32_t phys = 0U;
  Storage_Status st;

  if ((offset % NOR_FLASH_BLOCK64_SIZE) != 0U) {
    return STORAGE_ERR_RANGE;
  }
  st = MapPart(map, partition, offset, NOR_FLASH_BLOCK64_SIZE, &phys, 1U);
  if (st != STORAGE_OK) {
    return st;
  }
  StorageBackend_Lock(map->backend);
  st = StorageBackend_CheckVoltage(map->backend);
  if (st == STORAGE_OK) {
    if (map->backend->erase_block64 == NULL) {
      st = STORAGE_ERR_IO;
    } else {
      st = map->backend->erase_block64(map->backend->ctx, phys);
    }
  }
  StorageBackend_Unlock(map->backend);
  return st;
}

Storage_Status Storage_ErasePartition(const StoragePartitionMap *map,
                                      uint32_t partition)
{
  uint32_t offset;
  Storage_Status st;

  if ((map == NULL) || (partition >= map->part_count)) {
    return STORAGE_ERR_PARAM;
  }
  if (map->parts[partition].readonly != 0U) {
    return STORAGE_ERR_READONLY;
  }
  for (offset = 0U; offset < map->parts[partition].size;
       offset += NOR_FLASH_SECTOR_SIZE) {
    st = Storage_EraseSector(map, partition, offset);
    if (st != STORAGE_OK) {
      return st;
    }
  }
  return STORAGE_OK;
}
