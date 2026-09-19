#ifndef STM32TOOLS_BOOTLOADER_MEMMAP_H
#define STM32TOOLS_BOOTLOADER_MEMMAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Product configuration is authoritative when supplied. Defaults below remain
 * a legacy F411 example, not the Agriculture product's source of truth. */
#ifdef BOOTLOADER_CONFIG_HEADER
#include BOOTLOADER_CONFIG_HEADER
#if !defined(BOOTLOADER_FLASH_BASE) || !defined(BOOTLOADER_FLASH_SIZE) || \
    !defined(BOOTLOADER_APP_FLASH_BASE) || !defined(BOOTLOADER_APP_FLASH_SIZE)
#error "Product boot configuration must define all four internal Flash regions"
#endif
#endif
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

#ifndef BOOTLOADER_SRAM_BASE
#define BOOTLOADER_SRAM_BASE 0x20000000UL
#endif
#ifndef BOOTLOADER_SRAM_SIZE
#define BOOTLOADER_SRAM_SIZE (128UL * 1024UL)
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
