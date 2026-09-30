/* Real installer and boot policy, with deterministic flash/storage/HAL doubles.
 * Map a fixed MCU address into host memory for the uint32_t address API. */
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include "Bootloader/bootloader.h"
#include "Bootloader/bootloader_flash.h"
#include "Bootloader/bootloader_memmap.h"
#include "Bootloader/bootloader_policy.h"
#include "Common.h"
#include "stm32f4xx_hal.h"

#define IMAGE_SIZE 768U
#define SLOT_SIZE (2U * STORAGE_FW_MANIFEST_SIZE)
#define PART_CANDIDATE 1U
#define PART_ROLLBACK 2U
#define EXIT_JUMP 1
#define EXIT_POWER_LOSS 2

_Static_assert(BOOTLOADER_POLICY_ERR_PHASE_LIMIT == BOOTLOADER_ERR_PHASE_LIMIT,
               "Policy phase-limit error must match the public status");
_Static_assert(BOOTLOADER_POLICY_ERR_WATCHDOG_STORM == BOOTLOADER_ERR_WATCHDOG_STORM,
               "Policy watchdog-storm error must match the public status");

typedef struct {
  uint8_t image[BOOTLOADER_APP_FLASH_SIZE];
  StorageFirmwareManifest manifest;
  uint8_t valid;
} TestSlot;

TestSysTick test_systick;
TestScb test_scb;
TestNvic test_nvic;
static uint8_t *internal_flash;
static TestSlot slots[3];
static StoragePartitionMap map;
static BootloaderConfig config;
/* This survives simulated MCU resets; per-boot counters/faults do not. */
static UpgradeStatePayload durable_state;
static jmp_buf boot_exit;
static uint32_t read_count, program_count, erase_count, append_count, jump_count;
static uint32_t fail_read, fail_program, corrupt_program, fail_append;
static uint32_t cut_program, cut_vector_bytes, cut_append;
static uint8_t cut_after_erase, vectors_committed;
static uint8_t partial_erase, fail_log_init, fail_log_get;
static uint32_t programmed_slot;

void TestBootloader_Jump(void)
{
  ++jump_count;
  assert(Bootloader_IsAppValid(config.app_flash_base, config.app_flash_size));
  longjmp(boot_exit, EXIT_JUMP);
}

void BootloaderFlash_SetFeed(BootloaderFlash_FeedFn fn) { (void)fn; }

BootloaderFlash_Status BootloaderFlash_Erase(uint32_t address, uint32_t length)
{
  assert(address == config.app_flash_base);
  assert(length <= config.app_flash_size);
  ++erase_count;
  if (partial_erase != 0U) {
    /* An interrupted sector erase need not clear the old vector words. */
    internal_flash[32] ^= 1U;
    if (partial_erase == 2U) longjmp(boot_exit, EXIT_POWER_LOSS);
    return BOOTLOADER_FLASH_ERR_HAL;
  }
  memset(internal_flash, 0xFF, config.app_flash_size);
  vectors_committed = 0U;
  if (cut_after_erase) longjmp(boot_exit, EXIT_POWER_LOSS);
  return BOOTLOADER_FLASH_OK;
}

BootloaderFlash_Status BootloaderFlash_Program(uint32_t address,
                                               const uint8_t *data,
                                               uint32_t length)
{
  uint32_t offset = address - config.app_flash_base;
  uint32_t i, written = length;
  ++program_count;
  assert(address >= config.app_flash_base);
  assert(offset + length <= config.app_flash_size);
  if (offset == 0U) {
    /* A vector commit must come after every payload byte has been copied. */
    assert(length == 8U);
    assert(memcmp(internal_flash + 8U, slots[programmed_slot].image + 8U,
                  slots[programmed_slot].manifest.image_length - 8U) == 0);
    if (cut_vector_bytes != 0U) written = cut_vector_bytes;
  } else {
    /* Never expose even the first two words during body programming. */
    assert(Bootloader_IsAppValid(config.app_flash_base,
                                 config.app_flash_size) == 0U);
  }
  if (program_count == fail_program) written = length / 2U;
  for (i = 0U; i < written; ++i) internal_flash[offset + i] &= data[i];
  if (program_count == corrupt_program) internal_flash[offset] ^= 1U;
  if ((program_count == cut_program) ||
      ((offset == 0U) && (cut_vector_bytes != 0U))) {
    longjmp(boot_exit, EXIT_POWER_LOSS);
  }
  if (program_count == fail_program) return BOOTLOADER_FLASH_ERR_HAL;
  if (offset == 0U) vectors_committed = 1U;
  return BOOTLOADER_FLASH_OK;
}

Storage_Status StorageUpgrade_Init(StorageUpgradeLog *log,
                                   const StoragePartitionMap *part_map,
                                   uint32_t partition, uint32_t partition_size)
{
  (void)part_map; (void)partition; (void)partition_size;
  memset(log, 0, sizeof(*log));
  return fail_log_init ? STORAGE_ERR_INJECT : STORAGE_OK;
}
Storage_Status StorageUpgrade_Get(const StorageUpgradeLog *log,
                                  UpgradeStatePayload *out)
{
  (void)log;
  if (fail_log_get) return STORAGE_ERR_INJECT;
  *out = durable_state;
  return STORAGE_OK;
}
Storage_Status StorageUpgrade_Append(StorageUpgradeLog *log,
                                     const UpgradeStatePayload *payload)
{
  (void)log;
  if (++append_count == fail_append) return STORAGE_ERR_INJECT;
  durable_state = *payload;
  if (append_count == cut_append) longjmp(boot_exit, EXIT_POWER_LOSS);
  return STORAGE_OK;
}
Storage_Status StorageFirmware_InitSlot(StorageFirmwareSlot *slot,
                                        const StoragePartitionMap *part_map,
                                        uint32_t partition, uint32_t slot_size)
{
  assert(partition == PART_CANDIDATE || partition == PART_ROLLBACK);
  memset(slot, 0, sizeof(*slot));
  slot->map = part_map; slot->partition = partition; slot->slot_size = slot_size;
  return STORAGE_OK;
}
Storage_Status StorageFirmware_IsValid(StorageFirmwareSlot *slot,
                                       StorageFirmwareManifest *manifest)
{
  if (!slots[slot->partition].valid) return STORAGE_ERR_CRC;
  if (manifest != NULL) *manifest = slots[slot->partition].manifest;
  return STORAGE_OK;
}
Storage_Status StorageFirmware_ReadImage(StorageFirmwareSlot *slot,
                                         uint32_t offset, void *buffer,
                                         uint32_t length)
{
  if (++read_count == fail_read) return STORAGE_ERR_INJECT;
  programmed_slot = slot->partition;
  assert(offset + length <= slots[slot->partition].manifest.image_length);
  memcpy(buffer, slots[slot->partition].image + offset, length);
  return STORAGE_OK;
}
uint32_t StorageFirmware_ImageCapacity(uint32_t slot_size)
{
  return slot_size - STORAGE_FW_MANIFEST_SIZE;
}
Storage_Status StorageFirmware_BeginWrite(StorageFirmwareSlot *slot,
                                          uint32_t image_length)
{
  assert(image_length <= sizeof(slots[slot->partition].image));
  slots[slot->partition].valid = 0U;
  slot->write_offset = 0U;
  return STORAGE_OK;
}
Storage_Status StorageFirmware_WriteChunk(StorageFirmwareSlot *slot,
                                          const void *data, uint32_t length)
{
  assert(slot->write_offset + length <= sizeof(slots[slot->partition].image));
  memcpy(slots[slot->partition].image + slot->write_offset, data, length);
  slot->write_offset += length;
  return STORAGE_OK;
}
Storage_Status StorageFirmware_Finish(StorageFirmwareSlot *slot,
                                      const StorageFirmwareManifest *meta,
                                      StorageFirmwareManifest *out)
{
  slots[slot->partition].manifest = *meta;
  slots[slot->partition].manifest.magic = STORAGE_FW_MANIFEST_MAGIC;
  slots[slot->partition].valid = 1U;
  if (out != NULL) *out = slots[slot->partition].manifest;
  return STORAGE_OK;
}

static uint32_t Crc(const uint8_t *bytes, uint32_t size)
{
  return CalCRC32Update(0xFFFFFFFFUL, bytes, size) ^ 0xFFFFFFFFUL;
}

static void ResetBoot(void)
{
  read_count = program_count = erase_count = append_count = jump_count = 0U;
  fail_read = fail_program = corrupt_program = fail_append = 0U;
  cut_program = cut_vector_bytes = cut_append = 0U;
  cut_after_erase = vectors_committed = 0U;
  partial_erase = fail_log_init = fail_log_get = 0U;
  programmed_slot = PART_ROLLBACK;
}

static void Setup(void)
{
  uint32_t vectors[2] = {BOOTLOADER_SRAM_BASE + BOOTLOADER_SRAM_SIZE,
                         BOOTLOADER_APP_FLASH_BASE + 0x101U};
  TestSlot *rollback = &slots[PART_ROLLBACK];
  uint32_t i;
  ResetBoot();
  memset(&config, 0, sizeof(config));
  config.map = &map;
  config.app_flash_base = BOOTLOADER_APP_FLASH_BASE;
  config.app_flash_size = BOOTLOADER_APP_FLASH_SIZE;
  config.candidate_part = PART_CANDIDATE;
  config.candidate_part_size = SLOT_SIZE;
  config.rollback_part = PART_ROLLBACK;
  config.rollback_part_size = SLOT_SIZE;
  config.control_part_size = 4U * STORAGE_FW_MANIFEST_SIZE;
  config.reset_flags = BOOTLOADER_RST_POR;
  config.max_phase_attempts = 3U;
  memset(slots, 0, sizeof(slots));
  for (i = 0U; i < IMAGE_SIZE; ++i) rollback->image[i] = (uint8_t)(i ^ 0xA5U);
  memcpy(rollback->image, vectors, sizeof(vectors));
  rollback->manifest.magic = STORAGE_FW_MANIFEST_MAGIC;
  rollback->manifest.image_length = IMAGE_SIZE;
  rollback->manifest.target_address = config.app_flash_base;
  rollback->manifest.entry_address = vectors[1];
  rollback->manifest.image_crc32 = Crc(rollback->image, IMAGE_SIZE);
  rollback->manifest.firmware_version = 10U;
  rollback->valid = 1U;
  memset(internal_flash, 0x55, config.app_flash_size);
  memcpy(internal_flash, vectors, sizeof(vectors));
  memset(&durable_state, 0, sizeof(durable_state));
  durable_state.state = UPGRADE_STATE_ROLLBACK_PENDING;
  durable_state.active_version = 20U;
}

/* Returns the trap reason, or 0 with the normal returned boot status. */
static int Run(Bootloader_Status *status)
{
  int reason = setjmp(boot_exit);
  if (reason != 0) return reason;
  *status = Bootloader_Run(&config);
  return 0;
}

static void AssertRecovered(void)
{
  Bootloader_Status status = BOOTLOADER_OK;
  ResetBoot();
  assert(Run(&status) == EXIT_JUMP);
  assert(jump_count == 1U);
  assert(durable_state.state == UPGRADE_STATE_ROLLED_BACK);
  assert(durable_state.active_version == 10U);
  assert(durable_state.phase_attempts == 0U);
  assert(vectors_committed != 0U);
  assert(memcmp(internal_flash, slots[PART_ROLLBACK].image, IMAGE_SIZE) == 0);
}

static void TestRollbackIoFailure(uint8_t program_failure)
{
  Bootloader_Status status = BOOTLOADER_OK;
  Setup();
  if (program_failure) fail_program = 2U; else fail_read = 2U;
  assert(Run(&status) == 0);
  assert(status == (program_failure ? BOOTLOADER_ERR_FLASH : BOOTLOADER_ERR_STORAGE));
  assert(jump_count == 0U && erase_count == 1U && program_count >= 1U);
  assert(durable_state.state == UPGRADE_STATE_ROLLING_BACK);
  assert(durable_state.phase_attempts == 1U);
  assert(Bootloader_IsAppValid(config.app_flash_base, config.app_flash_size) == 0U);
  assert(memcmp(internal_flash + 8U, slots[PART_ROLLBACK].image + 8U, 248U) == 0);
  AssertRecovered();
}

static void TestDirectFailureNextBoot(uint8_t program_failure)
{
  StorageFirmwareSlot slot;
  Bootloader_Status status;
  Setup();
  assert(StorageFirmware_InitSlot(&slot, &map, PART_ROLLBACK, SLOT_SIZE) == STORAGE_OK);
  if (program_failure) fail_program = 2U; else fail_read = 2U;
  status = Bootloader_InstallSlot(&slot, &slots[PART_ROLLBACK].manifest,
                                  config.app_flash_base, config.app_flash_size);
  assert(status == (program_failure ? BOOTLOADER_ERR_FLASH : BOOTLOADER_ERR_STORAGE));
  /* Even fallback boot without recovery metadata must reject this image. */
  durable_state.state = UPGRADE_STATE_FAILED;
  ResetBoot();
  assert(Run(&status) == 0);
  assert(status == BOOTLOADER_ERR_NO_APP && jump_count == 0U && erase_count == 0U);
}

static void TestPowerLossRecovery(void)
{
  uint32_t point;
  Bootloader_Status status;
  for (point = 0U; point <= 5U; ++point) {
    Setup();
    if (point == 0U) cut_after_erase = 1U;
    else if (point == 5U) cut_vector_bytes = 4U;
    else cut_program = point;
    assert(Run(&status) == EXIT_POWER_LOSS);
    assert(durable_state.state == UPGRADE_STATE_ROLLING_BACK);
    assert(jump_count == 0U);
    /* Point 4 cuts after the final vector commit: its entire body is sound. */
    if (point != 4U) {
      assert(Bootloader_IsAppValid(config.app_flash_base, config.app_flash_size) == 0U);
    } else {
      assert(memcmp(internal_flash, slots[PART_ROLLBACK].image, IMAGE_SIZE) == 0);
    }
    AssertRecovered();
  }
}

static void TestRetryExhaustion(void)
{
  uint32_t attempt;
  Bootloader_Status status;
  Setup();
  for (attempt = 1U; attempt <= config.max_phase_attempts; ++attempt) {
    ResetBoot(); fail_program = 2U;
    assert(Run(&status) == 0 && status == BOOTLOADER_ERR_FLASH);
    assert(durable_state.state == UPGRADE_STATE_ROLLING_BACK);
    assert(durable_state.phase_attempts == attempt && jump_count == 0U);
  }
  /* Cover interrupted images left by old vector-first bootloader versions. */
  memcpy(internal_flash, slots[PART_ROLLBACK].image, 8U);
  assert(Bootloader_IsAppValid(config.app_flash_base, config.app_flash_size));
  for (attempt = 0U; attempt < 2U; ++attempt) {
    ResetBoot();
    assert(Run(&status) == 0 && status == BOOTLOADER_ERR_NO_APP);
    assert(jump_count == 0U && erase_count == 0U);
    assert(durable_state.state == UPGRADE_STATE_ROLLING_BACK);
    assert(durable_state.phase_attempts == config.max_phase_attempts);
  }
}

static void TestJournalFailure(void)
{
  uint32_t point;
  Bootloader_Status status;
  for (point = 1U; point <= 3U; ++point) {
    Setup(); fail_append = point;
    if (point == 3U) fail_read = 2U;
    assert(Run(&status) == 0);
    assert(jump_count == 0U);
    if (point <= 2U) {
      assert(status == BOOTLOADER_ERR_STORAGE && erase_count == 0U);
    } else {
      assert(status == BOOTLOADER_ERR_STORAGE && erase_count == 1U);
    }
    assert(durable_state.state == (point == 1U ? UPGRADE_STATE_ROLLBACK_PENDING :
                                                UPGRADE_STATE_ROLLING_BACK));
    AssertRecovered();
  }
}

static void TestInvalidRollbackNeverJumps(void)
{
  Bootloader_Status status;
  Setup(); slots[PART_ROLLBACK].valid = 0U;
  assert(Run(&status) == 0 && status == BOOTLOADER_ERR_ROLLBACK);
  assert(jump_count == 0U && erase_count == 0U);
  assert(durable_state.state == UPGRADE_STATE_ROLLING_BACK);
  slots[PART_ROLLBACK].valid = 1U;
  AssertRecovered();
}

static void TestVerificationBeforeCommit(void)
{
  Bootloader_Status status;
  Setup(); slots[PART_ROLLBACK].manifest.image_crc32 ^= 1U;
  assert(Run(&status) == 0 && status == BOOTLOADER_ERR_MANIFEST);
  assert(jump_count == 0U && vectors_committed == 0U && program_count == 3U);
  assert(Bootloader_IsAppValid(config.app_flash_base, config.app_flash_size) == 0U);
  slots[PART_ROLLBACK].manifest.image_crc32 ^= 1U;
  AssertRecovered();

  Setup(); corrupt_program = 2U;
  assert(Run(&status) == 0 && status == BOOTLOADER_ERR_FLASH);
  assert(jump_count == 0U && vectors_committed == 0U && program_count == 2U);
  assert(Bootloader_IsAppValid(config.app_flash_base, config.app_flash_size) == 0U);
  AssertRecovered();

  Setup(); fail_program = 4U; /* Half-written vector commit. */
  assert(Run(&status) == 0 && status == BOOTLOADER_ERR_FLASH);
  assert(jump_count == 0U && vectors_committed == 0U);
  assert(Bootloader_IsAppValid(config.app_flash_base, config.app_flash_size) == 0U);
  AssertRecovered();
}

static void TestShortManifestRejectedBeforeErase(void)
{
  Bootloader_Status status;
  Setup(); slots[PART_ROLLBACK].manifest.image_length = 7U;
  assert(Run(&status) == 0 && status == BOOTLOADER_ERR_MANIFEST);
  assert(erase_count == 0U && jump_count == 0U);
}

static void TestPartialEraseRecovery(void)
{
  uint32_t fault;
  Bootloader_Status status;
  for (fault = 1U; fault <= 2U; ++fault) {
    Setup(); partial_erase = (uint8_t)fault;
    assert(Run(&status) == (fault == 2U ? EXIT_POWER_LOSS : 0));
    if (fault == 1U) assert(status == BOOTLOADER_ERR_FLASH);
    assert(jump_count == 0U);
    assert(durable_state.state == UPGRADE_STATE_ROLLING_BACK);
    assert(Bootloader_IsAppValid(config.app_flash_base, config.app_flash_size));
    /* Do not trust those vectors if the recovery log becomes unreadable. */
    ResetBoot(); fail_log_init = 1U;
    assert(Run(&status) == 0 && status == BOOTLOADER_ERR_STORAGE);
    assert(jump_count == 0U && erase_count == 0U);
    ResetBoot(); fail_log_get = 1U;
    assert(Run(&status) == 0 && status == BOOTLOADER_ERR_STORAGE);
    assert(jump_count == 0U && erase_count == 0U);
    AssertRecovered();
  }
}

static void TestInterruptedCandidateRecovery(void)
{
  Bootloader_Status status;
  uint32_t state;
  for (state = UPGRADE_STATE_BACKUP_VALID; state <= UPGRADE_STATE_INSTALLING; ++state) {
    Setup();
    durable_state.state = state;
    durable_state.phase_attempts = 1U;
    /* Simulate a torn earlier erase with still-plausible vectors. */
    internal_flash[32] ^= 1U;
    assert(Run(&status) == 0 && status == BOOTLOADER_ERR_MANIFEST);
    assert(jump_count == 0U && erase_count == 0U);
    assert(durable_state.state == UPGRADE_STATE_INSTALLING);
    ResetBoot();
    assert(Run(&status) == 0 && status == BOOTLOADER_ERR_MANIFEST);
    assert(jump_count == 0U && durable_state.state == UPGRADE_STATE_INSTALLING);
    AssertRecovered(); /* Candidate retry limit falls back to verified rollback. */
  }

  Setup();
  slots[PART_CANDIDATE] = slots[PART_ROLLBACK];
  durable_state.state = UPGRADE_STATE_INSTALLING;
  durable_state.phase_attempts = 1U;
  slots[PART_ROLLBACK].valid = 0U;
  fail_read = 2U;
  assert(Run(&status) == 0 && status == BOOTLOADER_ERR_STORAGE);
  assert(jump_count == 0U && durable_state.state == UPGRADE_STATE_INSTALLING);
  /* A plausible interrupted internal image must never become the backup. */
  assert(slots[PART_ROLLBACK].valid == 0U);
  ResetBoot();
  assert(Run(&status) == EXIT_JUMP);
  assert(durable_state.state == UPGRADE_STATE_TRIAL_BOOT);
  assert(memcmp(internal_flash, slots[PART_CANDIDATE].image, IMAGE_SIZE) == 0);
}

static void TestCandidateRollbackPowerLoss(void)
{
  Bootloader_Status status;
  Setup();
  slots[PART_CANDIDATE] = slots[PART_ROLLBACK];
  durable_state.state = UPGRADE_STATE_CANDIDATE_VALID;
  partial_erase = 1U;
  /* Policy, backup, install intent, then install-failure record. */
  cut_append = 4U;
  assert(Run(&status) == EXIT_POWER_LOSS);
  assert(durable_state.state == UPGRADE_STATE_INSTALLING);
  assert(jump_count == 0U && erase_count == 1U);
  assert(Bootloader_IsAppValid(config.app_flash_base, config.app_flash_size));
  ResetBoot(); slots[PART_CANDIDATE].valid = 0U;
  assert(Run(&status) == 0 && status == BOOTLOADER_ERR_MANIFEST);
  assert(jump_count == 0U && durable_state.state == UPGRADE_STATE_INSTALLING);
  ResetBoot();
  assert(Run(&status) == 0 && status == BOOTLOADER_ERR_MANIFEST);
  ResetBoot();
  assert(Run(&status) == EXIT_JUMP);
  assert(durable_state.state == UPGRADE_STATE_ROLLED_BACK);
  assert(durable_state.active_version == 20U);
  assert(memcmp(internal_flash, slots[PART_ROLLBACK].image, config.app_flash_size) == 0);
}

static void TestInterruptedCandidateJournalFailure(void)
{
  uint32_t point;
  Bootloader_Status status;
  for (point = 2U; point <= 3U; ++point) {
    Setup();
    slots[PART_CANDIDATE] = slots[PART_ROLLBACK];
    durable_state.state = UPGRADE_STATE_INSTALLING;
    durable_state.phase_attempts = 1U;
    internal_flash[32] ^= 1U;
    fail_append = point;
    assert(Run(&status) == 0 && status == BOOTLOADER_ERR_STORAGE);
    assert(jump_count == 0U && erase_count == 0U);
    assert(durable_state.state == (point == 2U ? UPGRADE_STATE_INSTALLING :
                                                UPGRADE_STATE_BACKUP_VALID));
    ResetBoot(); slots[PART_CANDIDATE].valid = 0U;
    assert(Run(&status) == 0 && status == BOOTLOADER_ERR_MANIFEST);
    assert(jump_count == 0U && durable_state.state == UPGRADE_STATE_INSTALLING);
    AssertRecovered();
  }
}

static void TestPreDestructiveFallback(void)
{
  Bootloader_Status status;
  Setup(); durable_state.state = UPGRADE_STATE_CANDIDATE_VALID;
  slots[PART_CANDIDATE].valid = 0U;
  assert(Run(&status) == EXIT_JUMP);
  assert(erase_count == 0U && jump_count == 1U);
  assert(durable_state.state == UPGRADE_STATE_FAILED);
}

int main(void)
{
  void *wanted = (void *)(uintptr_t)BOOTLOADER_APP_FLASH_BASE;
#ifdef _WIN32
  MEMORY_BASIC_INFORMATION region;
  internal_flash = VirtualAlloc(wanted, BOOTLOADER_APP_FLASH_SIZE,
                                 MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  assert(internal_flash == wanted);
  assert(VirtualQuery(internal_flash, &region, sizeof(region)) == sizeof(region));
  assert(region.BaseAddress == wanted && region.AllocationBase == wanted);
  assert(region.RegionSize >= BOOTLOADER_APP_FLASH_SIZE && region.State == MEM_COMMIT);
#else
  internal_flash = mmap(wanted, BOOTLOADER_APP_FLASH_SIZE,
                        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(internal_flash == wanted);
#endif
  TestRollbackIoFailure(0U);
  TestRollbackIoFailure(1U);
  TestDirectFailureNextBoot(0U);
  TestDirectFailureNextBoot(1U);
  TestPowerLossRecovery();
  TestRetryExhaustion();
  TestJournalFailure();
  TestInvalidRollbackNeverJumps();
  TestVerificationBeforeCommit();
  TestShortManifestRejectedBeforeErase();
  TestPartialEraseRecovery();
  TestInterruptedCandidateRecovery();
  TestPreDestructiveFallback();
  TestCandidateRollbackPowerLoss();
  TestInterruptedCandidateJournalFailure();
#ifdef _WIN32
  assert(VirtualFree(internal_flash, 0, MEM_RELEASE) != 0);
#else
  assert(munmap(internal_flash, BOOTLOADER_APP_FLASH_SIZE) == 0);
#endif
  puts("bootloader_install_test: OK");
  return 0;
}
