#ifndef STM32TOOLS_NOR_FLASH_H
#define STM32TOOLS_NOR_FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NOR_FLASH_PAGE_SIZE     256U
#define NOR_FLASH_SECTOR_SIZE   4096U
#define NOR_FLASH_BLOCK64_SIZE  65536U

typedef enum {
  NOR_FLASH_OK = 0,
  NOR_FLASH_ERR_PARAM,
  NOR_FLASH_ERR_IO,
  NOR_FLASH_ERR_TIMEOUT,
  NOR_FLASH_ERR_RANGE
} NorFlash_Status;

typedef enum {
  NOR_FLASH_ERASE_SECTOR = 0,
  NOR_FLASH_ERASE_BLOCK64
} NorFlash_EraseType;

uint32_t NorFlash_CapacityFromJedec(uint32_t jedec_id);
NorFlash_Status NorFlash_CheckRange(uint32_t capacity_bytes, uint32_t address,
                                    uint32_t length);
uint32_t NorFlash_PageChunk(uint32_t address, uint32_t remaining,
                            uint32_t page_size);
uint32_t NorFlash_EraseSize(NorFlash_EraseType type);
uint32_t NorFlash_AlignDown(uint32_t address, uint32_t alignment);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_NOR_FLASH_H */
