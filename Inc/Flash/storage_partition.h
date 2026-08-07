#ifndef STM32TOOLS_STORAGE_PARTITION_H
#define STM32TOOLS_STORAGE_PARTITION_H

#include <stdint.h>

#include "Flash/nor_flash.h"
#include "Flash/storage_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t base;
  uint32_t size;
  uint8_t readonly;
  uint8_t reserved[3];
} StoragePartDesc;

typedef struct {
  const StorageBackend *backend;
  const StoragePartDesc *parts;
  uint32_t part_count;
  uint32_t media_size;
} StoragePartitionMap;

Storage_Status StoragePartition_Init(StoragePartitionMap *map,
                                     const StorageBackend *backend,
                                     const StoragePartDesc *parts,
                                     uint32_t part_count,
                                     uint32_t media_size);

Storage_Status StoragePartition_ValidateTable(const StoragePartDesc *parts,
                                              uint32_t part_count,
                                              uint32_t media_size);

Storage_Status Storage_Read(const StoragePartitionMap *map, uint32_t partition,
                            uint32_t offset, void *buffer, uint32_t length);
Storage_Status Storage_Write(const StoragePartitionMap *map, uint32_t partition,
                             uint32_t offset, const void *buffer,
                             uint32_t length);
Storage_Status Storage_EraseSector(const StoragePartitionMap *map,
                                   uint32_t partition, uint32_t offset);
Storage_Status Storage_EraseBlock64(const StoragePartitionMap *map,
                                    uint32_t partition, uint32_t offset);
Storage_Status Storage_ErasePartition(const StoragePartitionMap *map,
                                      uint32_t partition);

Storage_Status Storage_PhysFromPart(const StoragePartitionMap *map,
                                    uint32_t partition, uint32_t offset,
                                    uint32_t length, uint32_t *phys_out);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_STORAGE_PARTITION_H */
