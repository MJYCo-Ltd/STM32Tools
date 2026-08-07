#include "QSPIFlash.h"

#include "Flash/nor_flash.h"

#define CMD_WRITE_ENABLE     0x06U
#define CMD_READ_STATUS1     0x05U
#define CMD_READ_STATUS2     0x35U
#define CMD_WRITE_STATUS2    0x31U
#define CMD_READ_ID          0x9FU
#define CMD_PAGE_PROGRAM     0x32U
#define CMD_FAST_READ_QUAD   0xEBU
#define CMD_SECTOR_ERASE     0x20U
#define CMD_BLOCK_ERASE_64K  0xD8U
#define CMD_CHIP_ERASE       0xC7U

QSPI_FlashInfo QSPI_Flash;

static NorFlash_Status ToNorStatus(HAL_StatusTypeDef status)
{
  if (status == HAL_OK) {
    return NOR_FLASH_OK;
  }
  return (status == HAL_TIMEOUT) ? NOR_FLASH_ERR_TIMEOUT : NOR_FLASH_ERR_IO;
}

static HAL_StatusTypeDef FromNorStatus(NorFlash_Status status)
{
  if (status == NOR_FLASH_OK) {
    return HAL_OK;
  }
  return (status == NOR_FLASH_ERR_TIMEOUT) ? HAL_TIMEOUT : HAL_ERROR;
}

static HAL_StatusTypeDef QSPI_WriteEnable(void)
{
  QSPI_CommandTypeDef command = {0};

  command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  command.Instruction = CMD_WRITE_ENABLE;
  command.AddressMode = QSPI_ADDRESS_NONE;
  command.DataMode = QSPI_DATA_NONE;
  return HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

static HAL_StatusTypeDef QSPI_WaitBusy(void)
{
  QSPI_CommandTypeDef command = {0};
  QSPI_AutoPollingTypeDef config = {0};

  command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  command.Instruction = CMD_READ_STATUS1;
  command.AddressMode = QSPI_ADDRESS_NONE;
  command.DataMode = QSPI_DATA_1_LINE;

  config.Match = 0U;
  config.Mask = 1U;
  config.MatchMode = QSPI_MATCH_MODE_AND;
  config.Interval = 0x10U;
  config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
  return HAL_QSPI_AutoPolling(&hqspi, &command, &config,
                              HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

static HAL_StatusTypeDef QSPI_EnableQuadMode(void)
{
  QSPI_CommandTypeDef command = {0};
  uint8_t status2;
  HAL_StatusTypeDef status;

  command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  command.Instruction = CMD_READ_STATUS2;
  command.AddressMode = QSPI_ADDRESS_NONE;
  command.DataMode = QSPI_DATA_1_LINE;
  command.NbData = 1U;

  status = HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status == HAL_OK) {
    status = HAL_QSPI_Receive(&hqspi, &status2,
                              HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  }
  if ((status != HAL_OK) || ((status2 & 0x02U) != 0U)) {
    return status;
  }

  status2 |= 0x02U;
  status = QSPI_WriteEnable();
  if (status != HAL_OK) {
    return status;
  }
  command.Instruction = CMD_WRITE_STATUS2;
  status = HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status == HAL_OK) {
    status = HAL_QSPI_Transmit(&hqspi, &status2,
                               HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  }
  return (status == HAL_OK) ? QSPI_WaitBusy() : status;
}

static HAL_StatusTypeDef QSPI_ReadIDValue(uint32_t *jedec_id)
{
  QSPI_CommandTypeDef command = {0};
  uint8_t id[3];
  HAL_StatusTypeDef status;

  if (jedec_id == NULL) {
    return HAL_ERROR;
  }
  command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  command.Instruction = CMD_READ_ID;
  command.AddressMode = QSPI_ADDRESS_NONE;
  command.DataMode = QSPI_DATA_1_LINE;
  command.NbData = sizeof(id);

  status = HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status == HAL_OK) {
    status = HAL_QSPI_Receive(&hqspi, id, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  }
  if (status == HAL_OK) {
    *jedec_id = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
  }
  return status;
}

static void QSPI_PrepareFastRead(QSPI_CommandTypeDef *command,
                                 uint32_t address, uint32_t length)
{
  *command = (QSPI_CommandTypeDef){0};
  command->InstructionMode = QSPI_INSTRUCTION_1_LINE;
  command->Instruction = CMD_FAST_READ_QUAD;
  command->AddressMode = QSPI_ADDRESS_4_LINES;
  command->AddressSize = QSPI_ADDRESS_24_BITS;
  command->Address = address;
  command->DataMode = QSPI_DATA_4_LINES;
  command->DummyCycles = 6U;
  command->NbData = length;
}

static NorFlash_Status QSPI_ReadBackend(uint32_t address, uint8_t *data,
                                        uint32_t length)
{
  QSPI_CommandTypeDef command;
  HAL_StatusTypeDef status;

  QSPI_PrepareFastRead(&command, address, length);
  status = HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status == HAL_OK) {
    status = HAL_QSPI_Receive(&hqspi, data, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  }
  return ToNorStatus(status);
}

static NorFlash_Status QSPI_ProgramPageBackend(uint32_t address,
                                               const uint8_t *data,
                                               uint32_t length)
{
  QSPI_CommandTypeDef command = {0};
  HAL_StatusTypeDef status;

  status = QSPI_WriteEnable();
  if (status != HAL_OK) {
    return ToNorStatus(status);
  }

  command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  command.Instruction = CMD_PAGE_PROGRAM;
  command.AddressMode = QSPI_ADDRESS_1_LINE;
  command.AddressSize = QSPI_ADDRESS_24_BITS;
  command.Address = address;
  command.DataMode = QSPI_DATA_4_LINES;
  command.NbData = length;

  status = HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  if (status == HAL_OK) {
    status = HAL_QSPI_Transmit(&hqspi, (uint8_t *)data,
                               HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  }
  if (status == HAL_OK) {
    status = QSPI_WaitBusy();
  }
  return ToNorStatus(status);
}

static NorFlash_Status QSPI_EraseBackend(NorFlash_EraseType type,
                                         uint32_t address)
{
  QSPI_CommandTypeDef command = {0};
  HAL_StatusTypeDef status;

  switch (type) {
  case NOR_FLASH_ERASE_SECTOR:
    command.Instruction = CMD_SECTOR_ERASE;
    break;
  case NOR_FLASH_ERASE_BLOCK64:
    command.Instruction = CMD_BLOCK_ERASE_64K;
    break;
  default:
    return NOR_FLASH_ERR_PARAM;
  }

  status = QSPI_WriteEnable();
  if (status != HAL_OK) {
    return ToNorStatus(status);
  }
  command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  command.AddressMode = QSPI_ADDRESS_1_LINE;
  command.AddressSize = QSPI_ADDRESS_24_BITS;
  command.Address = address;
  command.DataMode = QSPI_DATA_NONE;

  status = HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  return ToNorStatus((status == HAL_OK) ? QSPI_WaitBusy() : status);
}

static NorFlash_Status QSPI_EraseChipBackend(void)
{
  QSPI_CommandTypeDef command = {0};
  HAL_StatusTypeDef status;

  status = QSPI_WriteEnable();
  if (status != HAL_OK) {
    return ToNorStatus(status);
  }
  command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  command.Instruction = CMD_CHIP_ERASE;
  command.AddressMode = QSPI_ADDRESS_NONE;
  command.DataMode = QSPI_DATA_NONE;

  status = HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  return ToNorStatus((status == HAL_OK) ? QSPI_WaitBusy() : status);
}

uint32_t QSPI_Flash_ReadID(void)
{
  uint32_t id = 0U;

  (void)QSPI_ReadIDValue(&id);
  return id;
}

HAL_StatusTypeDef QSPI_Flash_Init(void)
{
  uint32_t capacity;
  HAL_StatusTypeDef status = QSPI_ReadIDValue(&QSPI_Flash.ID);

  if (status != HAL_OK) {
    return status;
  }
  capacity = NorFlash_CapacityFromJedec(QSPI_Flash.ID);
  if ((capacity == 0U) || (capacity > UINT32_C(0x01000000))) {
    return HAL_ERROR;
  }
  QSPI_Flash.FlashSize = capacity;
  QSPI_Flash.PageSize = NOR_FLASH_PAGE_SIZE;
  QSPI_Flash.SectorSize = NOR_FLASH_SECTOR_SIZE;
  return QSPI_EnableQuadMode();
}

HAL_StatusTypeDef QSPI_Flash_Read(uint32_t address, uint8_t *data,
                                  uint32_t length)
{
  NorFlash_Status status =
      NorFlash_CheckRange(QSPI_Flash.FlashSize, address, length);

  if ((status != NOR_FLASH_OK) || (data == NULL)) {
    return HAL_ERROR;
  }
  return FromNorStatus(QSPI_ReadBackend(address, data, length));
}

HAL_StatusTypeDef QSPI_Flash_Read_DMA(uint32_t address, uint8_t *data,
                                      uint32_t length)
{
  QSPI_CommandTypeDef command;
  HAL_StatusTypeDef status;

  if ((data == NULL) ||
      (NorFlash_CheckRange(QSPI_Flash.FlashSize, address, length) !=
       NOR_FLASH_OK)) {
    return HAL_ERROR;
  }
  QSPI_PrepareFastRead(&command, address, length);
  status = HAL_QSPI_Command(&hqspi, &command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  return (status == HAL_OK) ? HAL_QSPI_Receive_DMA(&hqspi, data) : status;
}

HAL_StatusTypeDef QSPI_Flash_Write(uint32_t address, const uint8_t *data,
                                   uint32_t length)
{
  NorFlash_Status status =
      NorFlash_CheckRange(QSPI_Flash.FlashSize, address, length);

  if ((status != NOR_FLASH_OK) || (data == NULL)) {
    return HAL_ERROR;
  }
  while (length > 0U) {
    const uint32_t chunk =
        NorFlash_PageChunk(address, length, NOR_FLASH_PAGE_SIZE);

    status = QSPI_ProgramPageBackend(address, data, chunk);
    if (status != NOR_FLASH_OK) {
      return FromNorStatus(status);
    }
    address += chunk;
    data += chunk;
    length -= chunk;
  }
  return HAL_OK;
}

static HAL_StatusTypeDef QSPI_Erase(NorFlash_EraseType type, uint32_t address)
{
  const uint32_t erase_size = NorFlash_EraseSize(type);
  NorFlash_Status status;

  address = NorFlash_AlignDown(address, erase_size);
  status = NorFlash_CheckRange(QSPI_Flash.FlashSize, address, erase_size);
  return (status == NOR_FLASH_OK)
             ? FromNorStatus(QSPI_EraseBackend(type, address))
             : FromNorStatus(status);
}

HAL_StatusTypeDef QSPI_Flash_EraseSector(uint32_t address)
{
  return QSPI_Erase(NOR_FLASH_ERASE_SECTOR, address);
}

HAL_StatusTypeDef QSPI_Flash_EraseBlock(uint32_t address)
{
  return QSPI_Erase(NOR_FLASH_ERASE_BLOCK64, address);
}

HAL_StatusTypeDef QSPI_Flash_EraseChip(void)
{
  return FromNorStatus(QSPI_EraseChipBackend());
}

HAL_StatusTypeDef QSPI_EnableMemoryMapped(void)
{
  QSPI_CommandTypeDef command;
  QSPI_MemoryMappedTypeDef config = {0};

  QSPI_PrepareFastRead(&command, 0U, 0U);
  config.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
  return HAL_QSPI_MemoryMapped(&hqspi, &command, &config);
}
