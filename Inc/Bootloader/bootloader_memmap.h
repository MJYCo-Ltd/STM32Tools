#ifndef STM32TOOLS_BOOTLOADER_MEMMAP_H
#define STM32TOOLS_BOOTLOADER_MEMMAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Reusable STM32 port: the product supplies the memory map explicitly. */
#ifdef STM32TOOLS_BOOT_CONFIG_HEADER
#include STM32TOOLS_BOOT_CONFIG_HEADER
#endif
#if !defined(BOOTLOADER_FLASH_BASE) || !defined(BOOTLOADER_FLASH_SIZE) || \
    !defined(BOOTLOADER_APP_FLASH_BASE) || !defined(BOOTLOADER_APP_FLASH_SIZE) || \
    !defined(BOOTLOADER_SRAM_BASE) || !defined(BOOTLOADER_SRAM_SIZE)
#error "Supply STM32TOOLS_BOOT_CONFIG_HEADER or explicit Bootloader memory macros"
#endif

#ifndef BOOTLOADER_MAX_TRIAL_BOOTS
#define BOOTLOADER_MAX_TRIAL_BOOTS 3U
#endif
#ifndef BOOTLOADER_MAX_PHASE_ATTEMPTS
#define BOOTLOADER_MAX_PHASE_ATTEMPTS 3U
#endif
#ifndef BOOTLOADER_MAX_WATCHDOG_STORM
#define BOOTLOADER_MAX_WATCHDOG_STORM 8U
#endif

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_BOOTLOADER_MEMMAP_H */
