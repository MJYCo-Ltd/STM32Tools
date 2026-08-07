#ifndef STM32TOOLS_STORAGE_FIRMWARE_H
#define STM32TOOLS_STORAGE_FIRMWARE_H

#include <stdint.h>

#include "Flash/storage_partition.h"
#include "Flash/nor_flash.h"
#include "Flash/storage_pack.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STORAGE_FW_MANIFEST_MAGIC   0x4D574653UL /* 'SFWM' */
#define STORAGE_FW_MANIFEST_VERSION 1U
#define STORAGE_FW_MANIFEST_SIZE    NOR_FLASH_SECTOR_SIZE

STORAGE_PACK_BEGIN
typedef struct STORAGE_STRUCT_PACKED {
  uint32_t magic;
  uint16_t manifest_version;
  uint16_t reserved0;
  uint32_t hardware_id;
  uint32_t firmware_version;
  uint32_t image_length;
  uint32_t target_address;
  uint32_t entry_address;
  uint32_t build_time;
  uint32_t image_crc32;
  uint8_t sha256[32];
  uint8_t signature[64];
  uint32_t header_crc32;
  uint32_t commit_marker;
} StorageFirmwareManifest;
STORAGE_PACK_END

typedef struct {
  const StoragePartitionMap *map;
  uint32_t partition;
  uint32_t slot_size;
  uint32_t write_offset; /* within image area */
  uint32_t expected_length;
  uint32_t running_crc;
  uint8_t writing;
} StorageFirmwareSlot;

Storage_Status StorageFirmware_InitSlot(StorageFirmwareSlot *slot,
                                        const StoragePartitionMap *map,
                                        uint32_t partition,
                                        uint32_t slot_size);

Storage_Status StorageFirmware_BeginWrite(StorageFirmwareSlot *slot,
                                          uint32_t image_length);
Storage_Status StorageFirmware_WriteChunk(StorageFirmwareSlot *slot,
                                          const void *data, uint32_t length);
Storage_Status StorageFirmware_Finish(StorageFirmwareSlot *slot,
                                      const StorageFirmwareManifest *meta_in,
                                      StorageFirmwareManifest *meta_out);

Storage_Status StorageFirmware_Invalidate(StorageFirmwareSlot *slot);
Storage_Status StorageFirmware_ReadManifest(StorageFirmwareSlot *slot,
                                            StorageFirmwareManifest *manifest);
Storage_Status StorageFirmware_IsValid(StorageFirmwareSlot *slot,
                                       StorageFirmwareManifest *manifest);
Storage_Status StorageFirmware_ReadImage(StorageFirmwareSlot *slot,
                                         uint32_t offset, void *buffer,
                                         uint32_t length);

uint32_t StorageFirmware_ImageCapacity(uint32_t slot_size);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_STORAGE_FIRMWARE_H */
