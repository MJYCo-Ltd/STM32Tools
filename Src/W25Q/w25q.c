/* Winbond W25Qxx standard SPI (1-1-1) backend. */
#include "W25Q/w25q.h"

#include <string.h>

#define CMD_WRITE_ENABLE     0x06U
#define CMD_READ_STATUS1     0x05U
#define CMD_READ_DATA        0x03U
#define CMD_PAGE_PROGRAM     0x02U
#define CMD_SECTOR_ERASE     0x20U
#define CMD_BLOCK_ERASE_64K  0xD8U
#define CMD_CHIP_ERASE       0xC7U
#define CMD_JEDEC_ID         0x9FU

#define SR1_BUSY 0x01U

#define SPI_TIMEOUT_MS        100U
#define BUSY_TIMEOUT_MS       5000U
#define CHIP_ERASE_TIMEOUT_MS 60000U

static void CsLow(const W25Q_Device *dev)
{
  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}

static void CsHigh(const W25Q_Device *dev)
{
  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

static NorFlash_Status SpiTxRx(W25Q_Device *dev, uint8_t *tx, uint8_t *rx,
                               uint16_t length)
{
  HAL_StatusTypeDef status;

  if ((tx != NULL) && (rx != NULL)) {
    status = HAL_SPI_TransmitReceive(dev->hspi, tx, rx, length, SPI_TIMEOUT_MS);
  } else if (tx != NULL) {
    status = HAL_SPI_Transmit(dev->hspi, tx, length, SPI_TIMEOUT_MS);
  } else if (rx != NULL) {
    status = HAL_SPI_Receive(dev->hspi, rx, length, SPI_TIMEOUT_MS);
  } else {
    return NOR_FLASH_ERR_PARAM;
  }

  if (status == HAL_TIMEOUT) {
    return NOR_FLASH_ERR_TIMEOUT;
  }
  return (status == HAL_OK) ? NOR_FLASH_OK : NOR_FLASH_ERR_IO;
}

static NorFlash_Status WriteEnable(W25Q_Device *dev)
{
  uint8_t command = CMD_WRITE_ENABLE;
  NorFlash_Status status;

  CsLow(dev);
  status = SpiTxRx(dev, &command, NULL, 1U);
  CsHigh(dev);
  return status;
}

static NorFlash_Status ReadStatus1(W25Q_Device *dev, uint8_t *status1)
{
  uint8_t tx[2] = {CMD_READ_STATUS1, 0xFFU};
  uint8_t rx[2] = {0U, 0U};
  NorFlash_Status status;

  CsLow(dev);
  status = SpiTxRx(dev, tx, rx, 2U);
  CsHigh(dev);
  if (status == NOR_FLASH_OK) {
    *status1 = rx[1];
  }
  return status;
}

static NorFlash_Status WaitBusy(W25Q_Device *dev, uint32_t timeout_ms)
{
  const uint32_t start = HAL_GetTick();
  uint8_t status1;
  NorFlash_Status status;

  for (;;) {
    status = ReadStatus1(dev, &status1);
    if (status != NOR_FLASH_OK) {
      return status;
    }
    if ((status1 & SR1_BUSY) == 0U) {
      return NOR_FLASH_OK;
    }
    if ((HAL_GetTick() - start) >= timeout_ms) {
      return NOR_FLASH_ERR_TIMEOUT;
    }
  }
}

static NorFlash_Status SpiRead(W25Q_Device *dev, uint32_t address,
                               uint8_t *data, uint32_t length)
{
  uint8_t header[4];
  uint32_t done = 0U;
  NorFlash_Status status;

  header[0] = CMD_READ_DATA;
  header[1] = (uint8_t)(address >> 16);
  header[2] = (uint8_t)(address >> 8);
  header[3] = (uint8_t)address;

  CsLow(dev);
  status = SpiTxRx(dev, header, NULL, sizeof(header));
  while ((status == NOR_FLASH_OK) && (done < length)) {
    uint32_t chunk = length - done;

    if (chunk > UINT16_MAX) {
      chunk = UINT16_MAX;
    }
    status = SpiTxRx(dev, NULL, &data[done], (uint16_t)chunk);
    done += chunk;
  }
  CsHigh(dev);
  return status;
}

static NorFlash_Status SpiProgramPage(W25Q_Device *dev, uint32_t address,
                                      const uint8_t *data, uint32_t length)
{
  uint8_t header[4];
  NorFlash_Status status = WriteEnable(dev);

  if (status != NOR_FLASH_OK) {
    return status;
  }

  header[0] = CMD_PAGE_PROGRAM;
  header[1] = (uint8_t)(address >> 16);
  header[2] = (uint8_t)(address >> 8);
  header[3] = (uint8_t)address;

  CsLow(dev);
  status = SpiTxRx(dev, header, NULL, sizeof(header));
  if (status == NOR_FLASH_OK) {
    status = SpiTxRx(dev, (uint8_t *)data, NULL, (uint16_t)length);
  }
  CsHigh(dev);
  return (status == NOR_FLASH_OK) ? WaitBusy(dev, BUSY_TIMEOUT_MS) : status;
}

static NorFlash_Status SpiErase(W25Q_Device *dev, NorFlash_EraseType type,
                                uint32_t address)
{
  uint8_t header[4];
  NorFlash_Status status = WriteEnable(dev);

  if (status != NOR_FLASH_OK) {
    return status;
  }
  switch (type) {
  case NOR_FLASH_ERASE_SECTOR:
    header[0] = CMD_SECTOR_ERASE;
    break;
  case NOR_FLASH_ERASE_BLOCK64:
    header[0] = CMD_BLOCK_ERASE_64K;
    break;
  default:
    return NOR_FLASH_ERR_PARAM;
  }
  header[1] = (uint8_t)(address >> 16);
  header[2] = (uint8_t)(address >> 8);
  header[3] = (uint8_t)address;

  CsLow(dev);
  status = SpiTxRx(dev, header, NULL, sizeof(header));
  CsHigh(dev);
  return (status == NOR_FLASH_OK) ? WaitBusy(dev, BUSY_TIMEOUT_MS) : status;
}

static NorFlash_Status SpiEraseChip(W25Q_Device *dev)
{
  uint8_t command = CMD_CHIP_ERASE;
  NorFlash_Status status = WriteEnable(dev);

  if (status != NOR_FLASH_OK) {
    return status;
  }
  CsLow(dev);
  status = SpiTxRx(dev, &command, NULL, 1U);
  CsHigh(dev);
  return (status == NOR_FLASH_OK) ? WaitBusy(dev, CHIP_ERASE_TIMEOUT_MS)
                                  : status;
}

static W25Q_Status FromNorStatus(NorFlash_Status status)
{
  switch (status) {
  case NOR_FLASH_OK:
    return W25Q_OK;
  case NOR_FLASH_ERR_PARAM:
    return W25Q_ERR_PARAM;
  case NOR_FLASH_ERR_TIMEOUT:
    return W25Q_ERR_TIMEOUT;
  case NOR_FLASH_ERR_RANGE:
    return W25Q_ERR_RANGE;
  case NOR_FLASH_ERR_IO:
  default:
    return W25Q_ERR_SPI;
  }
}

uint32_t W25Q_ReadID(W25Q_Device *dev)
{
  uint8_t tx[4] = {CMD_JEDEC_ID, 0xFFU, 0xFFU, 0xFFU};
  uint8_t rx[4] = {0U, 0U, 0U, 0U};
  NorFlash_Status status;

  if ((dev == NULL) || (dev->hspi == NULL)) {
    return 0U;
  }
  CsLow(dev);
  status = SpiTxRx(dev, tx, rx, sizeof(tx));
  CsHigh(dev);
  if (status != NOR_FLASH_OK) {
    return 0U;
  }
  return ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
}

W25Q_Status W25Q_Init(W25Q_Device *dev, SPI_HandleTypeDef *hspi,
                      GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
  uint32_t capacity;

  if ((dev == NULL) || (hspi == NULL) || (cs_port == NULL)) {
    return W25Q_ERR_PARAM;
  }

  memset(dev, 0, sizeof(*dev));
  dev->hspi = hspi;
  dev->cs_port = cs_port;
  dev->cs_pin = cs_pin;
  CsHigh(dev);

  dev->jedec_id = W25Q_ReadID(dev);
  capacity = NorFlash_CapacityFromJedec(dev->jedec_id);
  if ((dev->jedec_id == 0U) || (dev->jedec_id == UINT32_C(0xFFFFFF)) ||
      (capacity == 0U) || (capacity > UINT32_C(0x01000000))) {
    return W25Q_ERR_ID;
  }
  dev->capacity_bytes = capacity;
  return W25Q_OK;
}

W25Q_Status W25Q_Read(W25Q_Device *dev, uint32_t address, uint8_t *data,
                      uint32_t length)
{
  NorFlash_Status status;

  if (dev == NULL) {
    return W25Q_ERR_PARAM;
  }
  status = NorFlash_CheckRange(dev->capacity_bytes, address, length);
  if ((status != NOR_FLASH_OK) || (data == NULL)) {
    return FromNorStatus((data == NULL) ? NOR_FLASH_ERR_PARAM : status);
  }
  return FromNorStatus(SpiRead(dev, address, data, length));
}

W25Q_Status W25Q_Write(W25Q_Device *dev, uint32_t address,
                       const uint8_t *data, uint32_t length)
{
  NorFlash_Status status;

  if (dev == NULL) {
    return W25Q_ERR_PARAM;
  }
  status = NorFlash_CheckRange(dev->capacity_bytes, address, length);
  if ((status != NOR_FLASH_OK) || (data == NULL)) {
    return FromNorStatus((data == NULL) ? NOR_FLASH_ERR_PARAM : status);
  }
  while (length > 0U) {
    const uint32_t chunk =
        NorFlash_PageChunk(address, length, NOR_FLASH_PAGE_SIZE);

    status = SpiProgramPage(dev, address, data, chunk);
    if (status != NOR_FLASH_OK) {
      return FromNorStatus(status);
    }
    address += chunk;
    data += chunk;
    length -= chunk;
  }
  return W25Q_OK;
}

static W25Q_Status Erase(W25Q_Device *dev, NorFlash_EraseType type,
                         uint32_t address)
{
  const uint32_t erase_size = NorFlash_EraseSize(type);
  NorFlash_Status status;

  if (dev == NULL) {
    return W25Q_ERR_PARAM;
  }
  address = NorFlash_AlignDown(address, erase_size);
  status = NorFlash_CheckRange(dev->capacity_bytes, address, erase_size);
  return (status == NOR_FLASH_OK) ? FromNorStatus(SpiErase(dev, type, address))
                                  : FromNorStatus(status);
}

W25Q_Status W25Q_EraseSector(W25Q_Device *dev, uint32_t address)
{
  return Erase(dev, NOR_FLASH_ERASE_SECTOR, address);
}

W25Q_Status W25Q_EraseBlock64(W25Q_Device *dev, uint32_t address)
{
  return Erase(dev, NOR_FLASH_ERASE_BLOCK64, address);
}

W25Q_Status W25Q_EraseChip(W25Q_Device *dev)
{
  if (dev == NULL) {
    return W25Q_ERR_PARAM;
  }
  return FromNorStatus(SpiEraseChip(dev));
}
