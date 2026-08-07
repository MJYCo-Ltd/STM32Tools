/*
 ******************************************************************************
 * @file           : w25q.c
 * @brief          : W25Qxx standard SPI driver (W25Q16JV datasheet)
 ******************************************************************************
 */
#include "W25Q/w25q.h"

#include <string.h>

/* Instruction Set Table 1 — Standard SPI (W25Q16JV) */
#define CMD_WRITE_ENABLE 0x06U
#define CMD_WRITE_DISABLE 0x04U
#define CMD_READ_STATUS1 0x05U
#define CMD_READ_DATA 0x03U
#define CMD_FAST_READ 0x0BU
#define CMD_PAGE_PROGRAM 0x02U
#define CMD_SECTOR_ERASE 0x20U
#define CMD_BLOCK_ERASE_64K 0xD8U
#define CMD_CHIP_ERASE 0xC7U
#define CMD_JEDEC_ID 0x9FU

#define SR1_BUSY 0x01U
#define SR1_WEL 0x02U

#define SPI_TIMEOUT_MS 100U
#define BUSY_TIMEOUT_MS 5000U
#define CHIP_ERASE_TIMEOUT_MS 60000U

static void CsLow(const W25Q_Device *dev)
{
  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}

static void CsHigh(const W25Q_Device *dev)
{
  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

static W25Q_Status SpiTxRx(W25Q_Device *dev, uint8_t *tx, uint8_t *rx,
                           uint16_t len)
{
  HAL_StatusTypeDef st;

  if ((tx != NULL) && (rx != NULL)) {
    st = HAL_SPI_TransmitReceive(dev->hspi, tx, rx, len, SPI_TIMEOUT_MS);
  } else if (tx != NULL) {
    st = HAL_SPI_Transmit(dev->hspi, tx, len, SPI_TIMEOUT_MS);
  } else if (rx != NULL) {
    st = HAL_SPI_Receive(dev->hspi, rx, len, SPI_TIMEOUT_MS);
  } else {
    return W25Q_ERR_PARAM;
  }
  return (st == HAL_OK) ? W25Q_OK : W25Q_ERR_SPI;
}

static W25Q_Status WriteEnable(W25Q_Device *dev)
{
  uint8_t cmd = CMD_WRITE_ENABLE;
  W25Q_Status st;

  CsLow(dev);
  st = SpiTxRx(dev, &cmd, NULL, 1U);
  CsHigh(dev);
  return st;
}

static W25Q_Status ReadStatus1(W25Q_Device *dev, uint8_t *sr1)
{
  uint8_t tx[2] = {CMD_READ_STATUS1, 0xFFU};
  uint8_t rx[2] = {0};
  W25Q_Status st;

  CsLow(dev);
  st = SpiTxRx(dev, tx, rx, 2U);
  CsHigh(dev);
  if (st == W25Q_OK) {
    *sr1 = rx[1];
  }
  return st;
}

static W25Q_Status WaitBusy(W25Q_Device *dev, uint32_t timeout_ms)
{
  const uint32_t start = HAL_GetTick();
  uint8_t sr1 = 0U;

  for (;;) {
    if (ReadStatus1(dev, &sr1) != W25Q_OK) {
      return W25Q_ERR_SPI;
    }
    if ((sr1 & SR1_BUSY) == 0U) {
      return W25Q_OK;
    }
    if ((HAL_GetTick() - start) >= timeout_ms) {
      return W25Q_ERR_TIMEOUT;
    }
  }
}

static uint32_t CapacityFromJedec(uint32_t id)
{
  /* Capacity byte: 2^N bytes; W25Q16 = 0x15 → 2MB */
  const uint8_t cap = (uint8_t)(id & 0xFFU);
  if ((cap < 0x10U) || (cap > 0x20U)) {
    return 0U;
  }
  return (1UL << cap);
}

static W25Q_Status CheckRange(const W25Q_Device *dev, uint32_t addr,
                              uint32_t len)
{
  if ((dev == NULL) || (len == 0U)) {
    return W25Q_ERR_PARAM;
  }
  if ((addr >= dev->capacity_bytes) ||
      (len > (dev->capacity_bytes - addr))) {
    return W25Q_ERR_RANGE;
  }
  return W25Q_OK;
}

uint32_t W25Q_ReadID(W25Q_Device *dev)
{
  uint8_t tx[4] = {CMD_JEDEC_ID, 0xFFU, 0xFFU, 0xFFU};
  uint8_t rx[4] = {0};
  W25Q_Status st;

  if ((dev == NULL) || (dev->hspi == NULL)) {
    return 0U;
  }
  CsLow(dev);
  st = SpiTxRx(dev, tx, rx, 4U);
  CsHigh(dev);
  if (st != W25Q_OK) {
    return 0U;
  }
  return ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | (uint32_t)rx[3];
}

W25Q_Status W25Q_Init(W25Q_Device *dev, SPI_HandleTypeDef *hspi,
                      GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
  uint32_t id;
  uint32_t size;

  if ((dev == NULL) || (hspi == NULL) || (cs_port == NULL)) {
    return W25Q_ERR_PARAM;
  }

  memset(dev, 0, sizeof(*dev));
  dev->hspi = hspi;
  dev->cs_port = cs_port;
  dev->cs_pin = cs_pin;
  CsHigh(dev);

  id = W25Q_ReadID(dev);
  size = CapacityFromJedec(id);
  if ((id == 0U) || (id == 0xFFFFFFUL) || (size == 0U)) {
    return W25Q_ERR_ID;
  }

  /* Prefer Winbond EFh; still accept other vendors with valid capacity. */
  dev->jedec_id = id;
  dev->capacity_bytes = size;
  return W25Q_OK;
}

W25Q_Status W25Q_Read(W25Q_Device *dev, uint32_t addr, uint8_t *buf,
                      uint32_t len)
{
  uint8_t hdr[4];
  W25Q_Status st;
  uint32_t done = 0U;

  st = CheckRange(dev, addr, len);
  if (st != W25Q_OK) {
    return st;
  }
  if (buf == NULL) {
    return W25Q_ERR_PARAM;
  }

  hdr[0] = CMD_READ_DATA;
  hdr[1] = (uint8_t)((addr >> 16) & 0xFFU);
  hdr[2] = (uint8_t)((addr >> 8) & 0xFFU);
  hdr[3] = (uint8_t)(addr & 0xFFU);

  CsLow(dev);
  st = SpiTxRx(dev, hdr, NULL, 4U);
  while ((st == W25Q_OK) && (done < len)) {
    uint32_t chunk = len - done;
    if (chunk > 0xFFFFU) {
      chunk = 0xFFFFU;
    }
    st = SpiTxRx(dev, NULL, &buf[done], (uint16_t)chunk);
    done += chunk;
  }
  CsHigh(dev);
  return st;
}

static W25Q_Status PageProgram(W25Q_Device *dev, uint32_t addr,
                               const uint8_t *buf, uint32_t len)
{
  uint8_t hdr[4];
  W25Q_Status st;

  if ((len == 0U) || (len > W25Q_PAGE_SIZE)) {
    return W25Q_ERR_PARAM;
  }

  st = WriteEnable(dev);
  if (st != W25Q_OK) {
    return st;
  }

  hdr[0] = CMD_PAGE_PROGRAM;
  hdr[1] = (uint8_t)((addr >> 16) & 0xFFU);
  hdr[2] = (uint8_t)((addr >> 8) & 0xFFU);
  hdr[3] = (uint8_t)(addr & 0xFFU);

  CsLow(dev);
  st = SpiTxRx(dev, hdr, NULL, 4U);
  if (st == W25Q_OK) {
    st = SpiTxRx(dev, (uint8_t *)buf, NULL, (uint16_t)len);
  }
  CsHigh(dev);
  if (st != W25Q_OK) {
    return st;
  }
  return WaitBusy(dev, BUSY_TIMEOUT_MS);
}

W25Q_Status W25Q_Write(W25Q_Device *dev, uint32_t addr, const uint8_t *buf,
                       uint32_t len)
{
  W25Q_Status st = CheckRange(dev, addr, len);
  if (st != W25Q_OK) {
    return st;
  }
  if (buf == NULL) {
    return W25Q_ERR_PARAM;
  }

  while (len > 0U) {
    uint32_t page_off = addr % W25Q_PAGE_SIZE;
    uint32_t chunk = W25Q_PAGE_SIZE - page_off;
    if (chunk > len) {
      chunk = len;
    }
    st = PageProgram(dev, addr, buf, chunk);
    if (st != W25Q_OK) {
      return st;
    }
    addr += chunk;
    buf += chunk;
    len -= chunk;
  }
  return W25Q_OK;
}

static W25Q_Status EraseCmd(W25Q_Device *dev, uint8_t cmd, uint32_t addr,
                            uint32_t timeout_ms)
{
  uint8_t hdr[4];
  W25Q_Status st;

  if ((dev == NULL) || (addr >= dev->capacity_bytes)) {
    return W25Q_ERR_RANGE;
  }

  st = WriteEnable(dev);
  if (st != W25Q_OK) {
    return st;
  }

  hdr[0] = cmd;
  hdr[1] = (uint8_t)((addr >> 16) & 0xFFU);
  hdr[2] = (uint8_t)((addr >> 8) & 0xFFU);
  hdr[3] = (uint8_t)(addr & 0xFFU);

  CsLow(dev);
  st = SpiTxRx(dev, hdr, NULL, 4U);
  CsHigh(dev);
  if (st != W25Q_OK) {
    return st;
  }
  return WaitBusy(dev, timeout_ms);
}

W25Q_Status W25Q_EraseSector(W25Q_Device *dev, uint32_t addr)
{
  return EraseCmd(dev, CMD_SECTOR_ERASE, addr & ~(W25Q_SECTOR_SIZE - 1U),
                  BUSY_TIMEOUT_MS);
}

W25Q_Status W25Q_EraseBlock64(W25Q_Device *dev, uint32_t addr)
{
  return EraseCmd(dev, CMD_BLOCK_ERASE_64K, addr & ~(W25Q_BLOCK64_SIZE - 1U),
                  BUSY_TIMEOUT_MS);
}

W25Q_Status W25Q_EraseChip(W25Q_Device *dev)
{
  uint8_t cmd = CMD_CHIP_ERASE;
  W25Q_Status st;

  if (dev == NULL) {
    return W25Q_ERR_PARAM;
  }
  st = WriteEnable(dev);
  if (st != W25Q_OK) {
    return st;
  }
  CsLow(dev);
  st = SpiTxRx(dev, &cmd, NULL, 1U);
  CsHigh(dev);
  if (st != W25Q_OK) {
    return st;
  }
  return WaitBusy(dev, CHIP_ERASE_TIMEOUT_MS);
}
