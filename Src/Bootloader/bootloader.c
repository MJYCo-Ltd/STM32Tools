/*
 ******************************************************************************
 * @file           : bootloader.c
 * @brief          : External-Flash OTA install / trial-boot / jump helper
 ******************************************************************************
 */
#include "Bootloader/bootloader.h"
#include "Bootloader/bootloader_flash.h"

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

static Storage_Status AppendState(StorageUpgradeLog *log,
                                  const UpgradeStatePayload *payload)
{
  return StorageUpgrade_Append(log, payload);
}

static Bootloader_Status InstallFromPart(const BootloaderConfig *cfg,
                                         uint32_t part, uint32_t part_size,
                                         StorageFirmwareManifest *manifest_out)
{
  StorageFirmwareSlot slot;
  StorageFirmwareManifest manifest;
  Storage_Status st;
  Bootloader_Status bst;

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

Bootloader_Status Bootloader_Run(const BootloaderConfig *cfg)
{
  StorageUpgradeLog ulog;
  UpgradeStatePayload state;
  StorageFirmwareManifest installed;
  Storage_Status st;
  Bootloader_Status bst;
  uint32_t app_base;
  uint32_t app_size;
  uint32_t max_trial;

  if ((cfg == NULL) || (cfg->map == NULL)) {
    return BOOTLOADER_ERR_PARAM;
  }

  app_base = (cfg->app_flash_base != 0U) ? cfg->app_flash_base
                                         : BOOTLOADER_APP_FLASH_BASE;
  app_size = (cfg->app_flash_size != 0U) ? cfg->app_flash_size
                                         : BOOTLOADER_APP_FLASH_SIZE;
  max_trial = (cfg->max_trial_boots != 0U) ? cfg->max_trial_boots
                                           : BOOTLOADER_MAX_TRIAL_BOOTS;

  st = StorageUpgrade_Init(&ulog, cfg->map, cfg->control_part,
                           cfg->control_part_size);
  if (st != STORAGE_OK) {
    if (Bootloader_IsAppValid(app_base, app_size) != 0U) {
      Bootloader_JumpToApp(app_base);
    }
    return BOOTLOADER_ERR_STORAGE;
  }

  memset(&state, 0, sizeof(state));
  st = StorageUpgrade_Get(&ulog, &state);
  if (st == STORAGE_ERR_NOT_FOUND) {
    state.state = (uint32_t)UPGRADE_STATE_IDLE;
  } else if (st != STORAGE_OK) {
    if (Bootloader_IsAppValid(app_base, app_size) != 0U) {
      Bootloader_JumpToApp(app_base);
    }
    return BOOTLOADER_ERR_STORAGE;
  }

  /* Install candidate */
  if ((state.state == (uint32_t)UPGRADE_STATE_INSTALLING) ||
      (state.state == (uint32_t)UPGRADE_STATE_CANDIDATE_VALID)) {
    bst = InstallFromPart(cfg, cfg->candidate_part, cfg->candidate_part_size,
                          &installed);
    if (bst == BOOTLOADER_OK) {
      state.state = (uint32_t)UPGRADE_STATE_TRIAL_BOOT;
      state.active_version = installed.firmware_version;
      state.candidate_version = installed.firmware_version;
      state.candidate_length = installed.image_length;
      state.candidate_crc32 = installed.image_crc32;
      state.trial_boot_count = 1U;
      state.last_error = 0U;
      (void)AppendState(&ulog, &state);
      Bootloader_JumpToApp(app_base);
    }
    state.state = (uint32_t)UPGRADE_STATE_FAILED;
    state.last_error = (uint32_t)bst;
    (void)AppendState(&ulog, &state);
  }

  /* Trial boot watchdog */
  if (state.state == (uint32_t)UPGRADE_STATE_TRIAL_BOOT) {
    if (state.trial_boot_count >= max_trial) {
      state.state = (uint32_t)UPGRADE_STATE_ROLLBACK_PENDING;
      (void)AppendState(&ulog, &state);
    } else {
      state.trial_boot_count += 1U;
      (void)AppendState(&ulog, &state);
      if (Bootloader_IsAppValid(app_base, app_size) != 0U) {
        Bootloader_JumpToApp(app_base);
      }
      return BOOTLOADER_ERR_NO_APP;
    }
  }

  /* Rollback */
  if ((state.state == (uint32_t)UPGRADE_STATE_ROLLBACK_PENDING) ||
      (state.state == (uint32_t)UPGRADE_STATE_ROLLING_BACK)) {
    state.state = (uint32_t)UPGRADE_STATE_ROLLING_BACK;
    (void)AppendState(&ulog, &state);
    bst = InstallFromPart(cfg, cfg->rollback_part, cfg->rollback_part_size,
                          &installed);
    if (bst == BOOTLOADER_OK) {
      state.state = (uint32_t)UPGRADE_STATE_ROLLED_BACK;
      state.active_version = installed.firmware_version;
      state.trial_boot_count = 0U;
      state.last_error = 0U;
      (void)AppendState(&ulog, &state);
      Bootloader_JumpToApp(app_base);
    }
    state.state = (uint32_t)UPGRADE_STATE_FAILED;
    state.last_error = (uint32_t)BOOTLOADER_ERR_ROLLBACK;
    (void)AppendState(&ulog, &state);
  }

  if (Bootloader_IsAppValid(app_base, app_size) != 0U) {
    Bootloader_JumpToApp(app_base);
  }
  return BOOTLOADER_ERR_NO_APP;
}
