#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "Flash/nor_flash.h"
#include "Flash/storage_backend.h"
#include "Flash/storage_crc.h"
#include "Flash/storage_firmware.h"
#include "Flash/storage_partition.h"
#include "Flash/storage_record.h"
#include "Flash/storage_upgrade.h"
#include "Bootloader/bootloader_policy.h"

#define RAM_FLASH_SIZE (256UL * 1024UL)

static uint8_t g_ram[RAM_FLASH_SIZE];
static int g_fail_writes_after = -1;
static int g_write_count;
static int g_fail_erases_after = -1;
static int g_erase_count;
static int g_poll_count;

static void RamPoll(void *ctx)
{
  (void)ctx;
  ++g_poll_count;
}

static Storage_Status RamRead(void *ctx, uint32_t address, void *buffer,
                              uint32_t length)
{
  (void)ctx;
  if (NorFlash_CheckRange(RAM_FLASH_SIZE, address, length) != NOR_FLASH_OK) {
    return STORAGE_ERR_RANGE;
  }
  memcpy(buffer, &g_ram[address], length);
  return STORAGE_OK;
}

static Storage_Status RamWrite(void *ctx, uint32_t address, const void *buffer,
                               uint32_t length)
{
  const uint8_t *src = (const uint8_t *)buffer;
  uint32_t i;
  (void)ctx;
  if (NorFlash_CheckRange(RAM_FLASH_SIZE, address, length) != NOR_FLASH_OK) {
    return STORAGE_ERR_RANGE;
  }
  ++g_write_count;
  if ((g_fail_writes_after >= 0) && (g_write_count > g_fail_writes_after)) {
    return STORAGE_ERR_INJECT;
  }
  for (i = 0U; i < length; ++i) {
    g_ram[address + i] &= src[i];
  }
  return STORAGE_OK;
}

static Storage_Status RamEraseSector(void *ctx, uint32_t address)
{
  (void)ctx;
  address = NorFlash_AlignDown(address, NOR_FLASH_SECTOR_SIZE);
  if (NorFlash_CheckRange(RAM_FLASH_SIZE, address, NOR_FLASH_SECTOR_SIZE) !=
      NOR_FLASH_OK) {
    return STORAGE_ERR_RANGE;
  }
  ++g_erase_count;
  if ((g_fail_erases_after >= 0) && (g_erase_count > g_fail_erases_after)) {
    return STORAGE_ERR_INJECT;
  }
  memset(&g_ram[address], 0xFF, NOR_FLASH_SECTOR_SIZE);
  return STORAGE_OK;
}

static Storage_Status RamEraseBlock64(void *ctx, uint32_t address)
{
  uint32_t i;
  address = NorFlash_AlignDown(address, NOR_FLASH_BLOCK64_SIZE);
  for (i = 0U; i < NOR_FLASH_BLOCK64_SIZE; i += NOR_FLASH_SECTOR_SIZE) {
    Storage_Status st = RamEraseSector(ctx, address + i);
    if (st != STORAGE_OK) {
      return st;
    }
  }
  return STORAGE_OK;
}

static void ResetRam(void)
{
  memset(g_ram, 0xFF, sizeof(g_ram));
  g_fail_writes_after = -1;
  g_fail_erases_after = -1;
  g_write_count = 0;
  g_erase_count = 0;
  g_poll_count = 0;
}

static StorageBackend MakeBackend(void)
{
  StorageBackend b;
  memset(&b, 0, sizeof(b));
  b.read = RamRead;
  b.write = RamWrite;
  b.erase_sector = RamEraseSector;
  b.erase_block64 = RamEraseBlock64;
  b.poll = RamPoll;
  return b;
}

static void TestPartitionBounds(void)
{
  StorageBackend backend = MakeBackend();
  StoragePartDesc parts[2] = {
      {0U, 64U * 1024U, 0U, {0, 0, 0}},
      {64U * 1024U, 64U * 1024U, 1U, {0, 0, 0}},
  };
  StoragePartitionMap map;
  uint8_t buf[16];
  assert(StoragePartition_Init(&map, &backend, parts, 2U, RAM_FLASH_SIZE) ==
         STORAGE_OK);
  assert(Storage_Read(&map, 0U, 0U, buf, sizeof(buf)) == STORAGE_OK);
  assert(Storage_Read(&map, 0U, (64U * 1024U) - 8U, buf, 16U) ==
         STORAGE_ERR_RANGE);
  assert(Storage_Write(&map, 1U, 0U, buf, sizeof(buf)) == STORAGE_ERR_READONLY);
  assert(Storage_EraseSector(&map, 0U, 1U) == STORAGE_ERR_RANGE);
  /* overflow-style: huge length */
  assert(Storage_Read(&map, 0U, 0U, buf, 0U) == STORAGE_ERR_PARAM);
}

static void TestRecordCommitAndHalfWrite(void)
{
  StorageBackend backend = MakeBackend();
  StoragePartDesc parts[1] = {{0U, 8U * 1024U, 0U, {0, 0, 0}}};
  StoragePartitionMap map;
  StorageRecordLoc loc;
  uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  uint8_t out[8];
  uint32_t written = 0U;
  StorageRecordHeader hdr;

  ResetRam();
  assert(StoragePartition_Init(&map, &backend, parts, 1U, RAM_FLASH_SIZE) ==
         STORAGE_OK);
  assert(Storage_ErasePartition(&map, 0U) == STORAGE_OK);
  assert(StorageRecord_Append(&map, 0U, 0U, 8U * 1024U, 1U, payload,
                              sizeof(payload), &written) == STORAGE_OK);
  assert(StorageRecord_FindLatest(&map, 0U, 0U, 8U * 1024U, &loc, out,
                                  sizeof(out)) == STORAGE_OK);
  assert(loc.sequence == 1U);
  assert(memcmp(out, payload, sizeof(payload)) == 0);

  /* Corrupt commit marker -> ignored */
  hdr.commit_marker = 0U;
  assert(Storage_Write(&map, 0U,
                       written + (uint32_t)offsetof(StorageRecordHeader,
                                                    commit_marker),
                       &hdr.commit_marker, 4U) == STORAGE_OK);
  assert(StorageRecord_FindLatest(&map, 0U, 0U, 8U * 1024U, &loc, out,
                                  sizeof(out)) == STORAGE_ERR_NOT_FOUND);

  /* Sequence wrap: 0 is newer than 0xFFFFFFF0 */
  assert(Storage_SeqIsNewer(0U, 0xFFFFFFF0UL) == 1);
  assert(Storage_SeqIsNewer(0xFFFFFFF0UL, 0U) == 0);
}

static void TestUpgradeCompactionAndInject(void)
{
  StorageBackend backend = MakeBackend();
  StoragePartDesc parts[1] = {{0U, 32U * 1024U, 0U, {0, 0, 0}}};
  StoragePartitionMap map;
  StorageUpgradeLog ulog;
  UpgradeStatePayload p;
  UpgradeStatePayload got;
  uint32_t i;

  ResetRam();
  assert(StoragePartition_Init(&map, &backend, parts, 1U, RAM_FLASH_SIZE) ==
         STORAGE_OK);
  assert(Storage_ErasePartition(&map, 0U) == STORAGE_OK);
  assert(StorageUpgrade_Init(&ulog, &map, 0U, 32U * 1024U) == STORAGE_OK);

  memset(&p, 0, sizeof(p));
  p.state = (uint32_t)UPGRADE_STATE_DOWNLOADING;
  for (i = 0U; i < 40U; ++i) {
    p.candidate_version = i;
    assert(StorageUpgrade_Append(&ulog, &p) == STORAGE_OK);
  }
  assert(StorageUpgrade_Get(&ulog, &got) == STORAGE_OK);
  assert(got.candidate_version == 39U);

  /* Re-init after "power loss" */
  assert(StorageUpgrade_Init(&ulog, &map, 0U, 32U * 1024U) == STORAGE_OK);
  assert(StorageUpgrade_Get(&ulog, &got) == STORAGE_OK);
  assert(got.candidate_version == 39U);

  /* Inject write failure then recover */
  g_write_count = 0;
  g_fail_writes_after = 1;
  p.state = (uint32_t)UPGRADE_STATE_FAILED;
  p.last_error = 7U;
  assert(StorageUpgrade_Append(&ulog, &p) == STORAGE_ERR_INJECT);
  g_fail_writes_after = -1;
  assert(StorageUpgrade_Init(&ulog, &map, 0U, 32U * 1024U) == STORAGE_OK);
  assert(StorageUpgrade_Get(&ulog, &got) == STORAGE_OK);
  assert(got.candidate_version == 39U);
}

static void TestFirmwareManifestGate(void)
{
  StorageBackend backend = MakeBackend();
  StoragePartDesc parts[1] = {{0U, 64U * 1024U, 0U, {0, 0, 0}}};
  StoragePartitionMap map;
  StorageFirmwareSlot slot;
  StorageFirmwareManifest meta;
  uint8_t chunk[256];
  uint32_t i;

  ResetRam();
  assert(StoragePartition_Init(&map, &backend, parts, 1U, RAM_FLASH_SIZE) ==
         STORAGE_OK);
  assert(StorageFirmware_InitSlot(&slot, &map, 0U, 64U * 1024U) == STORAGE_OK);
  assert(StorageFirmware_BeginWrite(&slot, 512U) == STORAGE_OK);
  assert(g_poll_count > 0);
  g_poll_count = 0;
  memset(chunk, 0xA5, sizeof(chunk));
  for (i = 0U; i < 2U; ++i) {
    assert(StorageFirmware_WriteChunk(&slot, chunk, sizeof(chunk)) ==
           STORAGE_OK);
  }
  assert(g_poll_count >= 2);
  /* Manifest not committed => invalid */
  assert(StorageFirmware_IsValid(&slot, NULL) != STORAGE_OK);

  memset(&meta, 0, sizeof(meta));
  meta.hardware_id = 1U;
  meta.firmware_version = 2U;
  meta.image_length = 512U;
  meta.target_address = 0x08020000UL;
  meta.entry_address = 0x08020001UL;
  meta.image_crc32 = Storage_Crc32(chunk, sizeof(chunk)); /* wrong: only one */
  /* Fix CRC for full image */
  {
    uint8_t image[512];
    memset(image, 0xA5, sizeof(image));
    meta.image_crc32 = Storage_Crc32(image, sizeof(image));
  }
  g_poll_count = 0;
  assert(StorageFirmware_Finish(&slot, &meta, NULL) == STORAGE_OK);
  assert(StorageFirmware_IsValid(&slot, &meta) == STORAGE_OK);
  assert(g_poll_count > 0);

  /* Oversize reject */
  assert(StorageFirmware_BeginWrite(
             &slot, StorageFirmware_ImageCapacity(64U * 1024U) + 1U) ==
         STORAGE_ERR_RANGE);
}

static void TestAddOverflow(void)
{
  assert(NorFlash_CheckRange(100U, 90U, 20U) == NOR_FLASH_ERR_RANGE);
  assert(NorFlash_CheckRange(100U, 0U, 100U) == NOR_FLASH_OK);
  assert(NorFlash_CheckRange(0U, 0U, 1U) == NOR_FLASH_ERR_PARAM);
}

static void FillPolicyIn(BootloaderPolicyIn *in, uint32_t state,
                         uint32_t flags, uint8_t app_valid)
{
  memset(in, 0, sizeof(*in));
  in->state = state;
  in->reset_flags = flags;
  in->app_valid = app_valid;
  in->max_trial_boots = 3U;
  in->max_phase_attempts = 3U;
  in->max_watchdog_storm = 8U;
}

static void TestBootloaderPolicy(void)
{
  BootloaderPolicyIn in;
  BootloaderPolicyOut out;

  /* Idle + valid App → jump, no persist */
  FillPolicyIn(&in, UPGRADE_STATE_IDLE, BOOTLOADER_RST_POR, 1U);
  BootloaderPolicy_Decide(&in, &out);
  assert(out.action == BOOTLOADER_ACTION_JUMP);
  assert(out.persist == 0U);

  /* No App → hold */
  FillPolicyIn(&in, UPGRADE_STATE_IDLE, BOOTLOADER_RST_POR, 0U);
  BootloaderPolicy_Decide(&in, &out);
  assert(out.action == BOOTLOADER_ACTION_HOLD);

  /* Installing: persist attempts before erase */
  FillPolicyIn(&in, UPGRADE_STATE_INSTALLING, BOOTLOADER_RST_SFT, 1U);
  BootloaderPolicy_Decide(&in, &out);
  assert(out.action == BOOTLOADER_ACTION_INSTALL);
  assert(out.persist == 1U);
  assert(out.phase_attempts == 1U);
  assert(out.state == (uint32_t)UPGRADE_STATE_INSTALLING);

  /* Power loss after the backup was committed resumes candidate install. */
  FillPolicyIn(&in, UPGRADE_STATE_BACKUP_VALID, BOOTLOADER_RST_SFT, 1U);
  BootloaderPolicy_Decide(&in, &out);
  assert(out.action == BOOTLOADER_ACTION_INSTALL);
  assert(out.persist == 1U);
  assert(out.phase_attempts == 1U);
  assert(out.state == (uint32_t)UPGRADE_STATE_INSTALLING);

  /* IWDG during install does not clear attempts; third retry then rollback */
  FillPolicyIn(&in, UPGRADE_STATE_INSTALLING, BOOTLOADER_RST_IWDG, 0U);
  in.phase_attempts = 2U;
  in.watchdog_resets = 2U;
  BootloaderPolicy_Decide(&in, &out);
  assert(out.action == BOOTLOADER_ACTION_INSTALL);
  assert(out.phase_attempts == 3U);
  assert(out.watchdog_resets == 3U);

  FillPolicyIn(&in, UPGRADE_STATE_INSTALLING, BOOTLOADER_RST_IWDG, 0U);
  in.phase_attempts = 3U;
  in.watchdog_resets = 3U;
  BootloaderPolicy_Decide(&in, &out);
  assert(out.action == BOOTLOADER_ACTION_ROLLBACK);
  assert(out.state == (uint32_t)UPGRADE_STATE_ROLLBACK_PENDING);
  assert(out.phase_attempts == 0U);

  /* PINRSTF is also set on IWDG; must still count as IWDG, not pin-clear */
  FillPolicyIn(&in, UPGRADE_STATE_CONFIRMED,
               BOOTLOADER_RST_IWDG | BOOTLOADER_RST_PIN, 1U);
  in.watchdog_resets = 0U;
  BootloaderPolicy_Decide(&in, &out);
  assert(out.action == BOOTLOADER_ACTION_JUMP);
  assert(out.watchdog_resets == 1U);
  assert(out.persist == 1U);

  /* Confirmed App IWDG storm → rollback, not another jump */
  FillPolicyIn(&in, UPGRADE_STATE_CONFIRMED,
               BOOTLOADER_RST_IWDG | BOOTLOADER_RST_PIN, 1U);
  in.watchdog_resets = 7U;
  BootloaderPolicy_Decide(&in, &out);
  assert(out.action == BOOTLOADER_ACTION_ROLLBACK);
  assert(out.state == (uint32_t)UPGRADE_STATE_ROLLBACK_PENDING);
  assert(out.last_error == BOOTLOADER_POLICY_ERR_WATCHDOG_STORM);

  /* Trial boot: count then jump; exhausted → rollback */
  FillPolicyIn(&in, UPGRADE_STATE_TRIAL_BOOT, BOOTLOADER_RST_IWDG, 1U);
  in.trial_boot_count = 1U;
  BootloaderPolicy_Decide(&in, &out);
  assert(out.action == BOOTLOADER_ACTION_JUMP);
  assert(out.trial_boot_count == 2U);

  FillPolicyIn(&in, UPGRADE_STATE_TRIAL_BOOT, 0U, 1U);
  in.trial_boot_count = 3U;
  BootloaderPolicy_Decide(&in, &out);
  assert(out.action == BOOTLOADER_ACTION_ROLLBACK);

  /* POR clears storm counter */
  FillPolicyIn(&in, UPGRADE_STATE_CONFIRMED,
               BOOTLOADER_RST_POR | BOOTLOADER_RST_PIN, 1U);
  in.watchdog_resets = 5U;
  BootloaderPolicy_Decide(&in, &out);
  assert(out.watchdog_resets == 0U);
  assert(out.action == BOOTLOADER_ACTION_JUMP);
}

int main(void)
{
  TestAddOverflow();
  TestPartitionBounds();
  TestRecordCommitAndHalfWrite();
  TestUpgradeCompactionAndInject();
  TestFirmwareManifestGate();
  TestBootloaderPolicy();
  printf("storage_test: OK\n");
  return 0;
}
