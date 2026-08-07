#include "Flash/nor_flash.h"

uint32_t NorFlash_CapacityFromJedec(uint32_t jedec_id)
{
  const uint8_t capacity_code = (uint8_t)(jedec_id & 0xFFU);

  /* Capacity byte is log2(bytes). A uint32_t cannot represent 2^32. */
  if ((capacity_code < 0x10U) || (capacity_code >= 0x20U)) {
    return 0U;
  }
  return UINT32_C(1) << capacity_code;
}

NorFlash_Status NorFlash_CheckRange(uint32_t capacity_bytes, uint32_t address,
                                    uint32_t length)
{
  if ((capacity_bytes == 0U) || (length == 0U)) {
    return NOR_FLASH_ERR_PARAM;
  }
  if ((address >= capacity_bytes) || (length > (capacity_bytes - address))) {
    return NOR_FLASH_ERR_RANGE;
  }
  return NOR_FLASH_OK;
}

uint32_t NorFlash_PageChunk(uint32_t address, uint32_t remaining,
                            uint32_t page_size)
{
  uint32_t chunk;

  if ((remaining == 0U) || (page_size == 0U)) {
    return 0U;
  }
  chunk = page_size - (address % page_size);
  return (chunk < remaining) ? chunk : remaining;
}

uint32_t NorFlash_EraseSize(NorFlash_EraseType type)
{
  switch (type) {
  case NOR_FLASH_ERASE_SECTOR:
    return NOR_FLASH_SECTOR_SIZE;
  case NOR_FLASH_ERASE_BLOCK64:
    return NOR_FLASH_BLOCK64_SIZE;
  default:
    return 0U;
  }
}

uint32_t NorFlash_AlignDown(uint32_t address, uint32_t alignment)
{
  return (alignment == 0U) ? address : address - (address % alignment);
}
