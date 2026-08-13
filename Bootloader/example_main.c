/*
 ******************************************************************************
 * @file           : example_main.c
 * @brief          : Skeleton for an Agriculture Bootloader CubeMX project
 *
 * Copy this pattern into a bare-metal CubeMX app that:
 *  - uses Bootloader/STM32F411xx_BOOT.ld (FLASH 128KB @ 0x08000000)
 *  - links STM32Tools Bootloader + Flash + W25Q sources
 *  - initializes SPI2 + W25Q CS, then calls Bootloader_Run()
 ******************************************************************************
 */
#if 0 /* Enable in a real Bootloader Cube project */

#include "main.h"
#include "spi.h"

#include <string.h>

#include <Bootloader/bootloader.h>
#include <Bootloader/bootloader_memmap.h>
#include <Flash/storage_partition.h>
#include <W25Q/w25q_storage.h>

/* Product layout — keep identical to Agriculture StorageLayout.c */
enum {
  PART_CONTROL = 0,
  PART_CONFIG,
  PART_BOOT_LOG,
  PART_CANDIDATE,
  PART_ROLLBACK,
  PART_RESERVED,
  PART_FACTORY,
  PART_COUNT
};

static const StoragePartDesc g_parts[PART_COUNT] = {
    {0x000000UL, 64UL * 1024UL, 0U, {0, 0, 0}},
    {0x010000UL, 128UL * 1024UL, 0U, {0, 0, 0}},
    {0x030000UL, 256UL * 1024UL, 0U, {0, 0, 0}},
    {0x070000UL, 512UL * 1024UL, 0U, {0, 0, 0}},
    {0x0F0000UL, 512UL * 1024UL, 0U, {0, 0, 0}},
    {0x170000UL, 512UL * 1024UL, 0U, {0, 0, 0}},
    {0x1F0000UL, 64UL * 1024UL, 1U, {0, 0, 0}},
};

static W25Q_Device g_flash;
static StorageBackend g_backend;
static StoragePartitionMap g_map;

int main(void)
{
  BootloaderConfig cfg;
  Bootloader_Status st;

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_SPI2_Init();

  if (W25Q_Init(&g_flash, &hspi2, FLASH_CS_GPIO_Port, FLASH_CS_Pin) != W25Q_OK) {
    for (;;) {
    }
  }
  W25Q_InitStorageBackend(&g_backend, &g_flash, NULL, NULL, NULL);
  if (StoragePartition_Init(&g_map, &g_backend, g_parts, PART_COUNT,
                            2UL * 1024UL * 1024UL) != STORAGE_OK) {
    for (;;) {
    }
  }

  memset(&cfg, 0, sizeof(cfg));
  cfg.map = &g_map;
  cfg.control_part = PART_CONTROL;
  cfg.control_part_size = g_parts[PART_CONTROL].size;
  cfg.candidate_part = PART_CANDIDATE;
  cfg.candidate_part_size = g_parts[PART_CANDIDATE].size;
  cfg.rollback_part = PART_ROLLBACK;
  cfg.rollback_part_size = g_parts[PART_ROLLBACK].size;
  cfg.app_flash_base = BOOTLOADER_APP_FLASH_BASE;
  cfg.app_flash_size = BOOTLOADER_APP_FLASH_SIZE;
  cfg.max_trial_boots = BOOTLOADER_MAX_TRIAL_BOOTS;

  st = Bootloader_Run(&cfg);
  (void)st;
  for (;;) {
    /* No valid Application image */
  }
}

#endif
