#ifndef STM32TOOLS_BOOTLOADER_FLASH_H
#define STM32TOOLS_BOOTLOADER_FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BOOTLOADER_FLASH_OK = 0,
  BOOTLOADER_FLASH_ERR_PARAM,
  BOOTLOADER_FLASH_ERR_RANGE,
  BOOTLOADER_FLASH_ERR_HAL
} BootloaderFlash_Status;

typedef void (*BootloaderFlash_FeedFn)(void);

void BootloaderFlash_SetFeed(BootloaderFlash_FeedFn fn);

/**
 * MCU-port: erase then program a contiguous Application region in internal Flash.
 * Erase is one sector at a time so IWDG can be fed between sectors.
 * Implementations must refuse addresses inside the Bootloader region.
 */
BootloaderFlash_Status BootloaderFlash_Erase(uint32_t address, uint32_t length);
BootloaderFlash_Status BootloaderFlash_Program(uint32_t address,
                                               const uint8_t *data,
                                               uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_BOOTLOADER_FLASH_H */
