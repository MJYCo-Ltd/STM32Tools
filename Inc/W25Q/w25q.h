/*
 ******************************************************************************
 * @file           : w25q.h
 * @brief          : Winbond W25Qxx SPI NOR Flash (standard SPI 1-1-1)
 *
 * Based on W25Q16JV datasheet (16M-bit / 2M-byte). Also accepts common
 * JEDEC IDs for W25Q32/64/128 when capacity byte differs.
 *
 * NOT for STM32 QSPI peripheral — use QSPIFlash on H7/etc. instead.
 ******************************************************************************
 */
#ifndef STM32TOOLS_W25Q_H
#define STM32TOOLS_W25Q_H

#include <stdint.h>

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define W25Q_PAGE_SIZE 256U
#define W25Q_SECTOR_SIZE 4096U
#define W25Q_BLOCK32_SIZE 32768U
#define W25Q_BLOCK64_SIZE 65536U

/** W25Q16JV: manufacturer EFh, memory type 40h, capacity 15h */
#define W25Q16_JEDEC_ID 0xEF4015UL

typedef enum {
  W25Q_OK = 0,
  W25Q_ERR_PARAM,
  W25Q_ERR_SPI,
  W25Q_ERR_TIMEOUT,
  W25Q_ERR_ID,
  W25Q_ERR_RANGE
} W25Q_Status;

typedef struct {
  SPI_HandleTypeDef *hspi;
  GPIO_TypeDef *cs_port;
  uint16_t cs_pin;
  uint32_t jedec_id;
  uint32_t capacity_bytes;
} W25Q_Device;

W25Q_Status W25Q_Init(W25Q_Device *dev, SPI_HandleTypeDef *hspi,
                      GPIO_TypeDef *cs_port, uint16_t cs_pin);
uint32_t W25Q_ReadID(W25Q_Device *dev);

W25Q_Status W25Q_Read(W25Q_Device *dev, uint32_t addr, uint8_t *buf,
                      uint32_t len);
W25Q_Status W25Q_Write(W25Q_Device *dev, uint32_t addr, const uint8_t *buf,
                       uint32_t len);

W25Q_Status W25Q_EraseSector(W25Q_Device *dev, uint32_t addr);
W25Q_Status W25Q_EraseBlock64(W25Q_Device *dev, uint32_t addr);
W25Q_Status W25Q_EraseChip(W25Q_Device *dev);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_W25Q_H */
