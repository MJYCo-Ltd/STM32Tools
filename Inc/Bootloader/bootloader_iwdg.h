#ifndef STM32TOOLS_BOOTLOADER_IWDG_H
#define STM32TOOLS_BOOTLOADER_IWDG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shared IWDG contract for Bootloader and Application.
 *
 * Timeout is sized for ONE STM32F411 128KB sector erase (max ~4s) plus LSI
 * spread, not for a multi-sector erase. Callers must feed between sectors.
 *
 * Prescaler 128, reload 4095, LSI ~32 kHz → ~16.4s typical
 * (≈11s at 47 kHz, ≈31s at 17 kHz).
 */
#define BOOTLOADER_IWDG_RELOAD 4095U

void BootloaderIwdg_Init(void);
void BootloaderIwdg_Feed(void);

/** Capture RCC_CSR (does not clear). Return portable BOOTLOADER_RST_* bits. */
uint32_t BootloaderIwdg_CaptureResetFlags(void);
void BootloaderIwdg_ClearResetFlags(void);
uint32_t BootloaderIwdg_ResetFlags(void);
uint8_t BootloaderIwdg_WasIwdgReset(void);

/**
 * Feed forever. Use when there is no safe App and retrying would only
 * produce a reset storm (missing image, exhausted rollback, storage dead).
 */
void BootloaderIwdg_SafeHold(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_BOOTLOADER_IWDG_H */
