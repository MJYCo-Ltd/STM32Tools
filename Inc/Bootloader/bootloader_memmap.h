#ifndef STM32TOOLS_BOOTLOADER_MEMMAP_H
#define STM32TOOLS_BOOTLOADER_MEMMAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Default internal Flash map for STM32F411 512KB (Agriculture board).
 * Bootloader occupies sectors 0..4 (128KB); Application sectors 5..7 (384KB).
 * Product headers (e.g. StorageLayout.h) should keep the same values.
 */
#ifndef BOOTLOADER_FLASH_BASE
#define BOOTLOADER_FLASH_BASE 0x08000000UL
#endif
#ifndef BOOTLOADER_FLASH_SIZE
#define BOOTLOADER_FLASH_SIZE (128UL * 1024UL)
#endif
#ifndef BOOTLOADER_APP_FLASH_BASE
#define BOOTLOADER_APP_FLASH_BASE 0x08020000UL
#endif
#ifndef BOOTLOADER_APP_FLASH_SIZE
#define BOOTLOADER_APP_FLASH_SIZE (384UL * 1024UL)
#endif

#ifndef BOOTLOADER_MAX_TRIAL_BOOTS
#define BOOTLOADER_MAX_TRIAL_BOOTS 3U
#endif

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_BOOTLOADER_MEMMAP_H */
