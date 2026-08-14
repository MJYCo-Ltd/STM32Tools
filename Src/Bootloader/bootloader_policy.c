#include <stddef.h>
#include "Bootloader/bootloader_policy.h"

#include "Flash/storage_upgrade.h"

static uint32_t SaturatingInc(uint32_t value)
{
  return (value == 0xFFFFFFFFUL) ? value : (value + 1U);
}

static void CopyIn(const BootloaderPolicyIn *in, BootloaderPolicyOut *out)
{
  out->action = BOOTLOADER_ACTION_JUMP;
  out->state = in->state;
  out->trial_boot_count = in->trial_boot_count;
  out->watchdog_resets = in->watchdog_resets;
  out->phase_attempts = in->phase_attempts;
  out->last_error = 0U;
  out->persist = 0U;
}

static void ApplyResetAccounting(const BootloaderPolicyIn *in,
                                 BootloaderPolicyOut *out)
{
  const uint32_t flags = in->reset_flags;
  const uint8_t por = ((flags & BOOTLOADER_RST_POR) != 0U) ? 1U : 0U;
  const uint8_t iwdg = ((flags & BOOTLOADER_RST_IWDG) != 0U) ? 1U : 0U;
  const uint8_t pin = ((flags & BOOTLOADER_RST_PIN) != 0U) ? 1U : 0U;
  const uint8_t sft = ((flags & BOOTLOADER_RST_SFT) != 0U) ? 1U : 0U;

  /* STM32 sets PINRSTF together with POR/IWDG/SFT. Inspect in that order. */
  if (por != 0U) {
    if (out->watchdog_resets != 0U) {
      out->watchdog_resets = 0U;
      out->persist = 1U;
    }
    return;
  }
  if (iwdg != 0U) {
    out->watchdog_resets = SaturatingInc(in->watchdog_resets);
    out->persist = 1U;
    return;
  }
  if ((pin != 0U) && (sft == 0U) && (out->watchdog_resets != 0U)) {
    out->watchdog_resets = 0U;
    out->persist = 1U;
  }
}

static void HoldOrJump(const BootloaderPolicyIn *in, BootloaderPolicyOut *out)
{
  if (in->app_valid != 0U) {
    out->action = BOOTLOADER_ACTION_JUMP;
  } else {
    out->action = BOOTLOADER_ACTION_HOLD;
    if (out->last_error == 0U) {
      out->last_error = BOOTLOADER_POLICY_ERR_NO_APP;
    }
  }
}

void BootloaderPolicy_Decide(const BootloaderPolicyIn *in,
                             BootloaderPolicyOut *out)
{
  uint32_t max_trial;
  uint32_t max_phase;
  uint32_t max_storm;

  if ((in == NULL) || (out == NULL)) {
    return;
  }

  CopyIn(in, out);
  max_trial = (in->max_trial_boots != 0U) ? in->max_trial_boots : 3U;
  max_phase = (in->max_phase_attempts != 0U) ? in->max_phase_attempts : 3U;
  max_storm = (in->max_watchdog_storm != 0U) ? in->max_watchdog_storm : 8U;

  ApplyResetAccounting(in, out);

  if ((in->state == (uint32_t)UPGRADE_STATE_INSTALLING) ||
      (in->state == (uint32_t)UPGRADE_STATE_CANDIDATE_VALID) ||
      (in->state == (uint32_t)UPGRADE_STATE_BACKUP_VALID)) {
    if (out->phase_attempts >= max_phase) {
      out->state = (uint32_t)UPGRADE_STATE_ROLLBACK_PENDING;
      out->phase_attempts = 0U;
      out->last_error = BOOTLOADER_POLICY_ERR_PHASE_LIMIT;
      out->action = BOOTLOADER_ACTION_ROLLBACK;
      out->persist = 1U;
      return;
    }
    out->state = (uint32_t)UPGRADE_STATE_INSTALLING;
    out->phase_attempts = SaturatingInc(in->phase_attempts);
    out->action = BOOTLOADER_ACTION_INSTALL;
    out->persist = 1U;
    return;
  }

  if (in->state == (uint32_t)UPGRADE_STATE_TRIAL_BOOT) {
    if (in->trial_boot_count >= max_trial) {
      out->state = (uint32_t)UPGRADE_STATE_ROLLBACK_PENDING;
      out->phase_attempts = 0U;
      out->action = BOOTLOADER_ACTION_ROLLBACK;
      out->persist = 1U;
      return;
    }
    if (in->app_valid == 0U) {
      out->state = (uint32_t)UPGRADE_STATE_ROLLBACK_PENDING;
      out->phase_attempts = 0U;
      out->last_error = BOOTLOADER_POLICY_ERR_NO_APP;
      out->action = BOOTLOADER_ACTION_ROLLBACK;
      out->persist = 1U;
      return;
    }
    out->trial_boot_count = SaturatingInc(in->trial_boot_count);
    out->action = BOOTLOADER_ACTION_JUMP;
    out->persist = 1U;
    return;
  }

  if ((in->state == (uint32_t)UPGRADE_STATE_ROLLBACK_PENDING) ||
      (in->state == (uint32_t)UPGRADE_STATE_ROLLING_BACK)) {
    if (out->phase_attempts >= max_phase) {
      out->state = (uint32_t)UPGRADE_STATE_FAILED;
      out->last_error = BOOTLOADER_POLICY_ERR_PHASE_LIMIT;
      out->persist = 1U;
      HoldOrJump(in, out);
      return;
    }
    out->state = (uint32_t)UPGRADE_STATE_ROLLING_BACK;
    out->phase_attempts = SaturatingInc(in->phase_attempts);
    out->action = BOOTLOADER_ACTION_ROLLBACK;
    out->persist = 1U;
    return;
  }

  if (((in->reset_flags & BOOTLOADER_RST_IWDG) != 0U) &&
      (out->watchdog_resets >= max_storm)) {
    out->state = (uint32_t)UPGRADE_STATE_ROLLBACK_PENDING;
    out->phase_attempts = 0U;
    out->last_error = BOOTLOADER_POLICY_ERR_WATCHDOG_STORM;
    out->action = BOOTLOADER_ACTION_ROLLBACK;
    out->persist = 1U;
    return;
  }

  HoldOrJump(in, out);
}
