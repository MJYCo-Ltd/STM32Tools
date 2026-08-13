#ifndef STM32TOOLS_BOOTLOADER_H
#define STM32TOOLS_BOOTLOADER_H

#include <stdint.h>

#include "Bootloader/bootloader_memmap.h"
#include "Flash/storage_firmware.h"
#include "Flash/storage_partition.h"
#include "Flash/storage_upgrade.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BOOTLOADER_OK = 0,
  BOOTLOADER_ERR_PARAM,
  BOOTLOADER_ERR_NO_APP,
  BOOTLOADER_ERR_STORAGE,
  BOOTLOADER_ERR_MANIFEST,
  BOOTLOADER_ERR_FLASH,
  BOOTLOADER_ERR_ROLLBACK
} Bootloader_Status;

/**
 * Product wires an already-initialized partition map and part indices.
 * Layout defaults match Agriculture F411 + W25Q16.
 */
typedef struct {
  const StoragePartitionMap *map;
  uint32_t control_part;
  uint32_t control_part_size;
  uint32_t candidate_part;
  uint32_t candidate_part_size;
  uint32_t rollback_part;
  uint32_t rollback_part_size;
  uint32_t app_flash_base;
  uint32_t app_flash_size;
  uint32_t max_trial_boots;
} BootloaderConfig;

/** True if stack pointer / Reset_Handler at app_base look like a valid vector table. */
uint8_t Bootloader_IsAppValid(uint32_t app_base, uint32_t app_size);

/** Never returns on success. */
void Bootloader_JumpToApp(uint32_t app_base);

/**
 * Program one external firmware slot into internal Application Flash.
 * Erases [target, target+length) then copies image bytes.
 */
Bootloader_Status Bootloader_InstallSlot(StorageFirmwareSlot *slot,
                                         const StorageFirmwareManifest *manifest,
                                         uint32_t app_flash_base,
                                         uint32_t app_flash_size);

/**
 * Read upgrade log from external Flash, install/rollback if needed, then jump.
 * On fatal error with no valid app, returns a status (caller may hang/blink).
 */
Bootloader_Status Bootloader_Run(const BootloaderConfig *cfg);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_BOOTLOADER_H */
