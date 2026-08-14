/*
 ******************************************************************************
 * @file           : bootloader.c
 * @brief          : External-Flash OTA install / trial-boot / jump helper
 ******************************************************************************
 */
#include "Bootloader/bootloader.h"
#include "Bootloader/bootloader_flash.h"
#include "Bootloader/bootloader_policy.h"

#include "Common.h"
#include "stm32f4xx_hal.h"

#include <string.h>

#define BOOTLOADER_CHUNK_SIZE 256U

uint8_t Bootloader_IsAppValid(uint32_t app_base, uint32_t app_size)
{
  const uint32_t sp = *(volatile uint32_t *)app_base;
  const uint32_t reset = *(volatile uint32_t *)(app_base + 4U);
  const uint32_t sram_base = 0x20000000UL;
  const uint32_t sram_end = 0x20000000UL + (128UL * 1024UL);

  (void)app_size;
  if ((sp < sram_base) || (sp > sram_end)) {
    return 0U;
  }
  if ((reset < app_base) || (reset > (app_base + app_size)) ||
      ((reset & 1U) == 0U)) {
    return 0U;
  }
  return 1U;
}

void Bootloader_JumpToApp(uint32_t app_base)
{
  typedef void (*AppReset_t)(void);
  const uint32_t sp = *(volatile uint32_t *)app_base;
  const uint32_t reset = *(volatile uint32_t *)(app_base + 4U);
  const AppReset_t app_reset = (AppReset_t)reset;

  __disable_irq();
  HAL_DeInit();
  SysTick->CTRL = 0U;
  SysTick->LOAD = 0U;
  SysTick->VAL = 0U;
  SCB->VTOR = app_base;
  __set_MSP(sp);
  app_reset();
  for (;;) {
  }
}

Bootloader_Status Bootloader_InstallSlot(StorageFirmwareSlot *slot,
                                         const StorageFirmwareManifest *manifest,
                                         uint32_t app_flash_base,
                                         uint32_t app_flash_size)
{
  uint8_t chunk[BOOTLOADER_CHUNK_SIZE];
  uint32_t offset = 0U;
  uint32_t running = 0xFFFFFFFFUL;
  BootloaderFlash_Status fst;

  if ((slot == NULL) || (manifest == NULL)) {
    return BOOTLOADER_ERR_PARAM;
  }
  if (manifest->magic != STORAGE_FW_MANIFEST_MAGIC) {
    return BOOTLOADER_ERR_MANIFEST;
  }
  if ((manifest->target_address != app_flash_base) ||
      (manifest->image_length == 0U) ||
      (manifest->image_length > app_flash_size) ||
      (manifest->entry_address < app_flash_base) ||
      (manifest->entry_address >= (app_flash_base + app_flash_size)) ||
      ((manifest->entry_address & 1U) == 0U)) {
    return BOOTLOADER_ERR_MANIFEST;
  }

  fst = BootloaderFlash_Erase(app_flash_base, manifest->image_length);
  if (fst != BOOTLOADER_FLASH_OK) {
    return BOOTLOADER_ERR_FLASH;
  }

  while (offset < manifest->image_length) {
    uint32_t n = manifest->image_length - offset;
    Storage_Status st;

    if (n > BOOTLOADER_CHUNK_SIZE) {
      n = BOOTLOADER_CHUNK_SIZE;
    }
    st = StorageFirmware_ReadImage(slot, offset, chunk, n);
    if (st != STORAGE_OK) {
      return BOOTLOADER_ERR_STORAGE;
    }
    fst = BootloaderFlash_Program(app_flash_base + offset, chunk, n);
    if (fst != BOOTLOADER_FLASH_OK) {
      return BOOTLOADER_ERR_FLASH;
    }
    running = CalCRC32Update(running, chunk, n);
    offset += n;
  }

  if ((running ^ 0xFFFFFFFFUL) != manifest->image_crc32) {
    return BOOTLOADER_ERR_MANIFEST;
  }
  if (Bootloader_IsAppValid(app_flash_base, app_flash_size) == 0U) {
    return BOOTLOADER_ERR_NO_APP;
  }
  return BOOTLOADER_OK;
}

static void FeedCfg(const BootloaderConfig *cfg)
{
  if ((cfg != NULL) && (cfg->watchdog_feed != NULL)) {
    cfg->watchdog_feed();
  }
}

static Storage_Status AppendState(StorageUpgradeLog *log,
                                  const UpgradeStatePayload *payload)
{
  return StorageUpgrade_Append(log, payload);
}

static void ApplyPolicy(UpgradeStatePayload *state,
                        const BootloaderPolicyOut *out, uint32_t reset_flags)
{
  state->state = out->state;
  state->trial_boot_count = out->trial_boot_count;
  state->watchdog_resets = out->watchdog_resets;
  state->phase_attempts = out->phase_attempts;
  state->reset_reason = reset_flags;
  if (out->last_error != 0U) {
    state->last_error = out->last_error;
  }
}

static Bootloader_Status JumpOrHold(const BootloaderConfig *cfg,
                                    uint32_t app_base, uint32_t app_size)
{
  FeedCfg(cfg);
  if (Bootloader_IsAppValid(app_base, app_size) != 0U) {
    Bootloader_JumpToApp(app_base);
  }
  return BOOTLOADER_ERR_NO_APP;
}

static uint8_t SlotIsValid(const BootloaderConfig *cfg, uint32_t part,
                           uint32_t part_size)
{
  StorageFirmwareSlot slot;
  StorageFirmwareManifest manifest;

  if (StorageFirmware_InitSlot(&slot, cfg->map, part, part_size) !=
      STORAGE_OK) {
    return 0U;
  }
  return (StorageFirmware_IsValid(&slot, &manifest) == STORAGE_OK) ? 1U : 0U;
}

static Bootloader_Status BackupCurrentApplication(
    const BootloaderConfig *cfg, const UpgradeStatePayload *state,
    const StorageFirmwareManifest *candidate, uint32_t app_base,
    uint32_t app_size)
{
  StorageFirmwareSlot rollback;
  StorageFirmwareManifest manifest;
  uint8_t chunk[BOOTLOADER_CHUNK_SIZE];
  uint32_t offset = 0U;
  uint32_t running_crc = 0xFFFFFFFFUL;
  Storage_Status st;

  if ((cfg == NULL) || (state == NULL) || (candidate == NULL) ||
      (Bootloader_IsAppValid(app_base, app_size) == 0U)) {
    return BOOTLOADER_ERR_BACKUP;
  }
  st = StorageFirmware_InitSlot(&rollback, cfg->map, cfg->rollback_part,
                                cfg->rollback_part_size);
  if (st != STORAGE_OK) {
    return BOOTLOADER_ERR_STORAGE;
  }
  if (app_size > StorageFirmware_ImageCapacity(cfg->rollback_part_size)) {
    return BOOTLOADER_ERR_BACKUP;
  }

  FeedCfg(cfg);
  st = StorageFirmware_BeginWrite(&rollback, app_size);
  if (st != STORAGE_OK) {
    return BOOTLOADER_ERR_BACKUP;
  }
  while (offset < app_size) {
    uint32_t n = app_size - offset;
    if (n > sizeof(chunk)) {
      n = sizeof(chunk);
    }
    FeedCfg(cfg);
    memcpy(chunk, (const void *)(uintptr_t)(app_base + offset), n);
    st = StorageFirmware_WriteChunk(&rollback, chunk, n);
    if (st != STORAGE_OK) {
      return BOOTLOADER_ERR_BACKUP;
    }
    running_crc = CalCRC32Update(running_crc, chunk, n);
    offset += n;
  }

  memset(&manifest, 0, sizeof(manifest));
  manifest.hardware_id = candidate->hardware_id;
  manifest.firmware_version = state->active_version;
  manifest.image_length = app_size;
  manifest.target_address = app_base;
  manifest.entry_address = *(volatile uint32_t *)(app_base + 4U);
  manifest.image_crc32 = running_crc ^ 0xFFFFFFFFUL;
  FeedCfg(cfg);
  st = StorageFirmware_Finish(&rollback, &manifest, NULL);
  if (st != STORAGE_OK) {
    return BOOTLOADER_ERR_BACKUP;
  }
  FeedCfg(cfg);
  st = StorageFirmware_IsValid(&rollback, &manifest);
  FeedCfg(cfg);
  return (st == STORAGE_OK) ? BOOTLOADER_OK : BOOTLOADER_ERR_BACKUP;
}

static Bootloader_Status EnsureRollbackBackup(
    const BootloaderConfig *cfg, const UpgradeStatePayload *state,
    const StorageFirmwareManifest *candidate, uint32_t app_base,
    uint32_t app_size)
{
  const uint8_t app_valid = Bootloader_IsAppValid(app_base, app_size);
  const uint8_t rollback_valid =
      SlotIsValid(cfg, cfg->rollback_part, cfg->rollback_part_size);

  /* The first install attempt must replace any stale rollback from an older
   * upgrade. On a retry, preserve a valid rollback: internal App may already
   * be partially erased by the interrupted install. */
  if ((state->phase_attempts > 1U) && (rollback_valid != 0U)) {
    return BOOTLOADER_OK;
  }
  if (app_valid == 0U) {
    return (rollback_valid != 0U) ? BOOTLOADER_OK : BOOTLOADER_ERR_NO_APP;
  }
  return BackupCurrentApplication(cfg, state, candidate, app_base, app_size);
}

static Bootloader_Status InstallFromPart(const BootloaderConfig *cfg,
                                         uint32_t part, uint32_t part_size,
                                         StorageFirmwareManifest *manifest_out)
{
  StorageFirmwareSlot slot;
  StorageFirmwareManifest manifest;
  Storage_Status st;
  Bootloader_Status bst;

  FeedCfg(cfg);
  st = StorageFirmware_InitSlot(&slot, cfg->map, part, part_size);
  if (st != STORAGE_OK) {
    return BOOTLOADER_ERR_STORAGE;
  }
  st = StorageFirmware_IsValid(&slot, &manifest);
  if (st != STORAGE_OK) {
    return BOOTLOADER_ERR_MANIFEST;
  }
  bst = Bootloader_InstallSlot(&slot, &manifest, cfg->app_flash_base,
                               cfg->app_flash_size);
  if ((bst == BOOTLOADER_OK) && (manifest_out != NULL)) {
    *manifest_out = manifest;
  }
  return bst;
}

static Storage_Status Persist(StorageUpgradeLog *log,
                              const UpgradeStatePayload *state)
{
  return AppendState(log, state);
}

static Bootloader_Status DoRollback(const BootloaderConfig *cfg,
                                    StorageUpgradeLog *log,
                                    UpgradeStatePayload *state,
                                    uint32_t app_base, uint32_t app_size)
{
  StorageFirmwareManifest installed;
  Bootloader_Status bst;

  if (SlotIsValid(cfg, cfg->rollback_part, cfg->rollback_part_size) == 0U) {
    state->state = (uint32_t)UPGRADE_STATE_FAILED;
    if (state->last_error == 0U) {
      state->last_error = (uint32_t)BOOTLOADER_ERR_ROLLBACK;
    }
    (void)Persist(log, state);
    /* A confirmed-but-crashing App with no rollback must not be jumped
     * again — that is the reset-storm case. */
    if (state->last_error == (uint32_t)BOOTLOADER_ERR_WATCHDOG_STORM) {
      return BOOTLOADER_ERR_NO_APP;
    }
    return JumpOrHold(cfg, app_base, app_size);
  }

  state->state = (uint32_t)UPGRADE_STATE_ROLLING_BACK;
  (void)Persist(log, state);
  bst = InstallFromPart(cfg, cfg->rollback_part, cfg->rollback_part_size,
                        &installed);
  if (bst == BOOTLOADER_OK) {
    state->state = (uint32_t)UPGRADE_STATE_ROLLED_BACK;
    state->active_version = installed.firmware_version;
    state->trial_boot_count = 0U;
    state->phase_attempts = 0U;
    state->watchdog_resets = 0U;
    state->last_error = 0U;
    (void)Persist(log, state);
    return JumpOrHold(cfg, app_base, app_size);
  }
  state->state = (uint32_t)UPGRADE_STATE_FAILED;
  state->last_error = (uint32_t)BOOTLOADER_ERR_ROLLBACK;
  (void)Persist(log, state);
  return JumpOrHold(cfg, app_base, app_size);
}

Bootloader_Status Bootloader_Run(const BootloaderConfig *cfg)
{
  StorageUpgradeLog ulog;
  UpgradeStatePayload state;
  StorageFirmwareManifest installed;
  Storage_Status st;
  Bootloader_Status bst;
  BootloaderPolicyIn pin;
  BootloaderPolicyOut pout;
  uint32_t app_base;
  uint32_t app_size;

  if ((cfg == NULL) || (cfg->map == NULL)) {
    return BOOTLOADER_ERR_PARAM;
  }

  BootloaderFlash_SetFeed(cfg->watchdog_feed);
  FeedCfg(cfg);

  app_base = (cfg->app_flash_base != 0U) ? cfg->app_flash_base
                                         : BOOTLOADER_APP_FLASH_BASE;
  app_size = (cfg->app_flash_size != 0U) ? cfg->app_flash_size
                                         : BOOTLOADER_APP_FLASH_SIZE;

  st = StorageUpgrade_Init(&ulog, cfg->map, cfg->control_part,
                           cfg->control_part_size);
  if (st != STORAGE_OK) {
    return JumpOrHold(cfg, app_base, app_size);
  }

  memset(&state, 0, sizeof(state));
  st = StorageUpgrade_Get(&ulog, &state);
  if (st == STORAGE_ERR_NOT_FOUND) {
    state.state = (uint32_t)UPGRADE_STATE_IDLE;
  } else if (st != STORAGE_OK) {
    return JumpOrHold(cfg, app_base, app_size);
  }

  memset(&pin, 0, sizeof(pin));
  pin.state = state.state;
  pin.trial_boot_count = state.trial_boot_count;
  pin.watchdog_resets = state.watchdog_resets;
  pin.phase_attempts = state.phase_attempts;
  pin.reset_flags = cfg->reset_flags;
  pin.app_valid = Bootloader_IsAppValid(app_base, app_size);
  pin.max_trial_boots = (cfg->max_trial_boots != 0U)
                            ? cfg->max_trial_boots
                            : BOOTLOADER_MAX_TRIAL_BOOTS;
  pin.max_phase_attempts = (cfg->max_phase_attempts != 0U)
                               ? cfg->max_phase_attempts
                               : BOOTLOADER_MAX_PHASE_ATTEMPTS;
  pin.max_watchdog_storm = (cfg->max_watchdog_storm != 0U)
                               ? cfg->max_watchdog_storm
                               : BOOTLOADER_MAX_WATCHDOG_STORM;

  memset(&pout, 0, sizeof(pout));
  BootloaderPolicy_Decide(&pin, &pout);
  ApplyPolicy(&state, &pout, cfg->reset_flags);

  if (pout.persist != 0U) {
    FeedCfg(cfg);
    if (Persist(&ulog, &state) != STORAGE_OK) {
      return JumpOrHold(cfg, app_base, app_size);
    }
  }

  if (pout.action == BOOTLOADER_ACTION_INSTALL) {
    StorageFirmwareSlot candidate_slot;
    uint8_t backup_available = 0U;

    st = StorageFirmware_InitSlot(&candidate_slot, cfg->map,
                                  cfg->candidate_part,
                                  cfg->candidate_part_size);
    if (st != STORAGE_OK) {
      state.state = (uint32_t)UPGRADE_STATE_FAILED;
      state.last_error = (uint32_t)BOOTLOADER_ERR_STORAGE;
      (void)Persist(&ulog, &state);
      return JumpOrHold(cfg, app_base, app_size);
    }
    st = StorageFirmware_IsValid(&candidate_slot, &installed);
    if (st != STORAGE_OK) {
      state.state = (uint32_t)UPGRADE_STATE_FAILED;
      state.last_error = (uint32_t)BOOTLOADER_ERR_MANIFEST;
      (void)Persist(&ulog, &state);
      return JumpOrHold(cfg, app_base, app_size);
    }

    bst = EnsureRollbackBackup(cfg, &state, &installed, app_base, app_size);
    if (bst == BOOTLOADER_OK) {
      backup_available = 1U;
      state.state = (uint32_t)UPGRADE_STATE_BACKUP_VALID;
      if (Persist(&ulog, &state) != STORAGE_OK) {
        return JumpOrHold(cfg, app_base, app_size);
      }
    } else if (Bootloader_IsAppValid(app_base, app_size) != 0U) {
      /* Never erase a valid running image when its backup could not be
       * committed and verified. */
      state.state = (uint32_t)UPGRADE_STATE_FAILED;
      state.last_error = (uint32_t)bst;
      (void)Persist(&ulog, &state);
      return JumpOrHold(cfg, app_base, app_size);
    }

    state.state = (uint32_t)UPGRADE_STATE_INSTALLING;
    if (Persist(&ulog, &state) != STORAGE_OK) {
      return JumpOrHold(cfg, app_base, app_size);
    }
    bst = Bootloader_InstallSlot(&candidate_slot, &installed, app_base,
                                 app_size);
    if (bst == BOOTLOADER_OK) {
      state.state = (uint32_t)UPGRADE_STATE_TRIAL_BOOT;
      state.active_version = installed.firmware_version;
      state.candidate_version = installed.firmware_version;
      state.candidate_length = installed.image_length;
      state.candidate_crc32 = installed.image_crc32;
      state.trial_boot_count = 1U;
      state.phase_attempts = 0U;
      state.watchdog_resets = 0U;
      state.last_error = 0U;
      (void)Persist(&ulog, &state);
      return JumpOrHold(cfg, app_base, app_size);
    }
    state.state = (uint32_t)UPGRADE_STATE_FAILED;
    state.last_error = (uint32_t)bst;
    (void)Persist(&ulog, &state);
    if (backup_available != 0U) {
      return DoRollback(cfg, &ulog, &state, app_base, app_size);
    }
    return JumpOrHold(cfg, app_base, app_size);
  }

  if (pout.action == BOOTLOADER_ACTION_ROLLBACK) {
    return DoRollback(cfg, &ulog, &state, app_base, app_size);
  }

  if (pout.action == BOOTLOADER_ACTION_HOLD) {
    return BOOTLOADER_ERR_NO_APP;
  }

  return JumpOrHold(cfg, app_base, app_size);
}
