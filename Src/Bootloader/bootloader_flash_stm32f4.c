/*
 ******************************************************************************
 * @file           : bootloader_flash_stm32f4.c
 * @brief          : Internal Flash erase/program for STM32F4 (F411 512KB map)
 ******************************************************************************
 */
#include "Bootloader/bootloader_flash.h"
#include "Bootloader/bootloader_memmap.h"

#include "stm32f4xx_hal.h"

#include <string.h>

static BootloaderFlash_FeedFn g_feed;

void BootloaderFlash_SetFeed(BootloaderFlash_FeedFn fn)
{
  g_feed = fn;
}

static void Feed(void)
{
  if (g_feed != NULL) {
    g_feed();
  }
}

static uint32_t SectorOf(uint32_t address)
{
  if (address < 0x08004000UL) {
    return FLASH_SECTOR_0;
  }
  if (address < 0x08008000UL) {
    return FLASH_SECTOR_1;
  }
  if (address < 0x0800C000UL) {
    return FLASH_SECTOR_2;
  }
  if (address < 0x08010000UL) {
    return FLASH_SECTOR_3;
  }
  if (address < 0x08020000UL) {
    return FLASH_SECTOR_4;
  }
  if (address < 0x08040000UL) {
    return FLASH_SECTOR_5;
  }
  if (address < 0x08060000UL) {
    return FLASH_SECTOR_6;
  }
  return FLASH_SECTOR_7;
}

static uint8_t RangeOk(uint32_t address, uint32_t length)
{
  const uint32_t app_begin = BOOTLOADER_APP_FLASH_BASE;
  const uint32_t app_end = BOOTLOADER_APP_FLASH_BASE + BOOTLOADER_APP_FLASH_SIZE;

  if ((length == 0U) || (address < app_begin)) {
    return 0U;
  }
  if ((address + length) < address) {
    return 0U;
  }
  if ((address + length) > app_end) {
    return 0U;
  }
  return 1U;
}

BootloaderFlash_Status BootloaderFlash_Erase(uint32_t address, uint32_t length)
{
  FLASH_EraseInitTypeDef erase;
  uint32_t sector_error = 0U;
  uint32_t first;
  uint32_t last;
  uint32_t sector;
  HAL_StatusTypeDef st = HAL_OK;

  if (RangeOk(address, length) == 0U) {
    return BOOTLOADER_FLASH_ERR_RANGE;
  }

  first = SectorOf(address);
  last = SectorOf(address + length - 1U);
  if (last < first) {
    return BOOTLOADER_FLASH_ERR_PARAM;
  }

  erase.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase.Banks = FLASH_BANK_1;
  erase.NbSectors = 1U;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  HAL_FLASH_Unlock();
  for (sector = first; sector <= last; ++sector) {
    Feed();
    erase.Sector = sector;
    st = HAL_FLASHEx_Erase(&erase, &sector_error);
    if (st != HAL_OK) {
      break;
    }
  }
  HAL_FLASH_Lock();
  Feed();
  return (st == HAL_OK) ? BOOTLOADER_FLASH_OK : BOOTLOADER_FLASH_ERR_HAL;
}

BootloaderFlash_Status BootloaderFlash_Program(uint32_t address,
                                               const uint8_t *data,
                                               uint32_t length)
{
  uint32_t offset = 0U;
  HAL_StatusTypeDef st = HAL_OK;

  if ((data == NULL) || (RangeOk(address, length) == 0U)) {
    return BOOTLOADER_FLASH_ERR_PARAM;
  }

  HAL_FLASH_Unlock();
  while ((offset + 4U) <= length) {
    uint32_t word;
    if ((offset & 0x3FFU) == 0U) {
      Feed();
    }
    memcpy(&word, &data[offset], sizeof(word));
    st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + offset, word);
    if (st != HAL_OK) {
      break;
    }
    offset += 4U;
  }
  while ((st == HAL_OK) && (offset < length)) {
    st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address + offset,
                           data[offset]);
    if (st != HAL_OK) {
      break;
    }
    offset += 1U;
  }
  HAL_FLASH_Lock();
  Feed();
  return (st == HAL_OK) ? BOOTLOADER_FLASH_OK : BOOTLOADER_FLASH_ERR_HAL;
}
