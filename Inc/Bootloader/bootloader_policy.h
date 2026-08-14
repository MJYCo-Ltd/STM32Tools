#ifndef STM32TOOLS_BOOTLOADER_POLICY_H
#define STM32TOOLS_BOOTLOADER_POLICY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Portable reset-cause bits (mapped from RCC_CSR on STM32).
 * Host tests use these directly; MCU code translates hardware flags.
 */
#define BOOTLOADER_RST_IWDG (1UL << 0)
#define BOOTLOADER_RST_PIN  (1UL << 1)
#define BOOTLOADER_RST_POR  (1UL << 2)
#define BOOTLOADER_RST_SFT  (1UL << 3)

/* Numeric values match Bootloader_Status in bootloader.h. */
#define BOOTLOADER_POLICY_ERR_NO_APP 2U
#define BOOTLOADER_POLICY_ERR_PHASE_LIMIT 7U
#define BOOTLOADER_POLICY_ERR_WATCHDOG_STORM 8U

typedef enum {
  BOOTLOADER_ACTION_JUMP = 0,
  BOOTLOADER_ACTION_INSTALL,
  BOOTLOADER_ACTION_ROLLBACK,
  BOOTLOADER_ACTION_HOLD
} BootloaderAction;

typedef struct {
  uint32_t state;
  uint32_t trial_boot_count;
  uint32_t watchdog_resets;
  uint32_t phase_attempts;
  uint32_t reset_flags;
  uint8_t app_valid;
  uint32_t max_trial_boots;
  uint32_t max_phase_attempts;
  uint32_t max_watchdog_storm;
} BootloaderPolicyIn;

typedef struct {
  BootloaderAction action;
  uint32_t state;
  uint32_t trial_boot_count;
  uint32_t watchdog_resets;
  uint32_t phase_attempts;
  uint32_t last_error;
  uint8_t persist;
} BootloaderPolicyOut;

/**
 * Decide the next Bootloader step from persisted upgrade state + reset cause.
 * Callers MUST persist `out` when `persist` is set, before erase / jump.
 */
void BootloaderPolicy_Decide(const BootloaderPolicyIn *in,
                             BootloaderPolicyOut *out);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_BOOTLOADER_POLICY_H */
