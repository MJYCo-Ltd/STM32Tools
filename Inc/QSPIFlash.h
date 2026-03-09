#ifndef __QSPI_FLASH_H
#define __QSPI_FLASH_H

#include "stm32h7xx_hal.h"

extern QSPI_HandleTypeDef hqspi;

#define QSPI_FLASH_BASE        0x90000000

#define W25Q_PAGE_SIZE         256
#define W25Q_SECTOR_SIZE       4096
#define W25Q_BLOCK_SIZE        65536

typedef struct
{
    uint32_t FlashSize;
    uint32_t SectorSize;
    uint32_t PageSize;
    uint32_t ID;
}QSPI_FlashInfo;

extern QSPI_FlashInfo QSPI_Flash;

HAL_StatusTypeDef QSPI_Flash_Init(void);
uint32_t QSPI_Flash_ReadID(void);

HAL_StatusTypeDef QSPI_Flash_Read(uint32_t addr,uint8_t *buf,uint32_t len);
HAL_StatusTypeDef QSPI_Flash_Read_DMA(uint32_t addr,uint8_t *buf,uint32_t len);

HAL_StatusTypeDef QSPI_Flash_Write(uint32_t addr,uint8_t *buf,uint32_t len);

HAL_StatusTypeDef QSPI_Flash_EraseSector(uint32_t addr);
HAL_StatusTypeDef QSPI_Flash_EraseBlock(uint32_t addr);
HAL_StatusTypeDef QSPI_Flash_EraseChip(void);

HAL_StatusTypeDef QSPI_EnableMemoryMapped(void);

#endif