#include <QSPIFlash.h>

QSPI_FlashInfo QSPI_Flash;

#define CMD_WRITE_ENABLE        0x06
#define CMD_READ_STATUS1        0x05
#define CMD_READ_STATUS2        0x35
#define CMD_WRITE_STATUS2       0x31
#define CMD_READ_ID             0x9F

#define CMD_PAGE_PROGRAM        0x32
#define CMD_FAST_READ_QUAD      0xEB

#define CMD_SECTOR_ERASE        0x20
#define CMD_BLOCK_ERASE         0xD8
#define CMD_CHIP_ERASE          0xC7

static HAL_StatusTypeDef QSPI_WriteEnable(void)
{
    QSPI_CommandTypeDef cmd={0};

    cmd.InstructionMode=QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction=CMD_WRITE_ENABLE;
    cmd.AddressMode=QSPI_ADDRESS_NONE;
    cmd.DataMode=QSPI_DATA_NONE;

    return HAL_QSPI_Command(&hqspi,&cmd,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

static HAL_StatusTypeDef QSPI_WaitBusy(void)
{
    QSPI_CommandTypeDef cmd={0};
    QSPI_AutoPollingTypeDef cfg={0};

    cmd.InstructionMode=QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction=CMD_READ_STATUS1;

    cmd.AddressMode=QSPI_ADDRESS_NONE;
    cmd.DataMode=QSPI_DATA_1_LINE;

    cfg.Match=0;
    cfg.Mask=1;
    cfg.MatchMode=QSPI_MATCH_MODE_AND;
    cfg.Interval=0x10;
    cfg.AutomaticStop=QSPI_AUTOMATIC_STOP_ENABLE;

    return HAL_QSPI_AutoPolling(&hqspi,&cmd,&cfg,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

static HAL_StatusTypeDef QSPI_EnableQuadMode(void)
{
    uint8_t sr2;

    QSPI_CommandTypeDef cmd={0};

    cmd.InstructionMode=QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction=CMD_READ_STATUS2;
    cmd.AddressMode=QSPI_ADDRESS_NONE;
    cmd.DataMode=QSPI_DATA_1_LINE;
    cmd.NbData=1;

    HAL_QSPI_Command(&hqspi,&cmd,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
    HAL_QSPI_Receive(&hqspi,&sr2,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

    if(!(sr2 & 0x02))
    {
        sr2|=0x02;

        QSPI_WriteEnable();

        cmd.Instruction=CMD_WRITE_STATUS2;

        HAL_QSPI_Command(&hqspi,&cmd,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
        HAL_QSPI_Transmit(&hqspi,&sr2,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

        QSPI_WaitBusy();
    }

    return HAL_OK;
}

uint32_t QSPI_Flash_ReadID(void)
{
    uint8_t id[3];

    QSPI_CommandTypeDef cmd={0};

    cmd.InstructionMode=QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction=CMD_READ_ID;
    cmd.AddressMode=QSPI_ADDRESS_NONE;
    cmd.DataMode=QSPI_DATA_1_LINE;
    cmd.NbData=3;

    HAL_QSPI_Command(&hqspi,&cmd,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
    HAL_QSPI_Receive(&hqspi,id,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

    return (id[0]<<16)|(id[1]<<8)|id[2];
}

HAL_StatusTypeDef QSPI_Flash_Init(void)
{
    uint32_t id=QSPI_Flash_ReadID();

    QSPI_Flash.ID=id;

    if((id & 0xFFFF)==0x4017)
        QSPI_Flash.FlashSize=8*1024*1024;

    if((id & 0xFFFF)==0x4018)
        QSPI_Flash.FlashSize=16*1024*1024;

    QSPI_Flash.PageSize=W25Q_PAGE_SIZE;
    QSPI_Flash.SectorSize=W25Q_SECTOR_SIZE;

    return QSPI_EnableQuadMode();
}

HAL_StatusTypeDef QSPI_Flash_Read(uint32_t addr,uint8_t *buf,uint32_t len)
{
    QSPI_CommandTypeDef cmd={0};

    cmd.InstructionMode=QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction=CMD_FAST_READ_QUAD;

    cmd.AddressMode=QSPI_ADDRESS_4_LINES;
    cmd.AddressSize=QSPI_ADDRESS_24_BITS;
    cmd.Address=addr;

    cmd.DataMode=QSPI_DATA_4_LINES;
    cmd.DummyCycles=6;
    cmd.NbData=len;

    HAL_QSPI_Command(&hqspi,&cmd,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

    return HAL_QSPI_Receive(&hqspi,buf,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

HAL_StatusTypeDef QSPI_Flash_Read_DMA(uint32_t addr,uint8_t *buf,uint32_t len)
{
    QSPI_CommandTypeDef cmd={0};

    cmd.InstructionMode=QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction=CMD_FAST_READ_QUAD;

    cmd.AddressMode=QSPI_ADDRESS_4_LINES;
    cmd.AddressSize=QSPI_ADDRESS_24_BITS;
    cmd.Address=addr;

    cmd.DataMode=QSPI_DATA_4_LINES;

    cmd.DummyCycles=6;
    cmd.NbData=len;

    HAL_QSPI_Command(&hqspi,&cmd,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

    return HAL_QSPI_Receive_DMA(&hqspi,buf);
}

HAL_StatusTypeDef QSPI_Flash_Write(uint32_t addr,uint8_t *buf,uint32_t len)
{
    QSPI_CommandTypeDef cmd={0};

    uint32_t current_addr=addr;
    uint32_t end_addr=addr+len;
    uint32_t size;

    while(current_addr<end_addr)
    {
        size=W25Q_PAGE_SIZE-(current_addr%W25Q_PAGE_SIZE);

        if(size>(end_addr-current_addr))
            size=end_addr-current_addr;

        QSPI_WriteEnable();

        cmd.InstructionMode=QSPI_INSTRUCTION_1_LINE;
        cmd.Instruction=CMD_PAGE_PROGRAM;

        cmd.AddressMode=QSPI_ADDRESS_1_LINE;
        cmd.AddressSize=QSPI_ADDRESS_24_BITS;
        cmd.Address=current_addr;

        cmd.DataMode=QSPI_DATA_4_LINES;
        cmd.NbData=size;

        HAL_QSPI_Command(&hqspi,&cmd,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
        HAL_QSPI_Transmit(&hqspi,buf,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

        QSPI_WaitBusy();

        current_addr+=size;
        buf+=size;
    }

    return HAL_OK;
}

HAL_StatusTypeDef QSPI_Flash_EraseSector(uint32_t addr)
{
    QSPI_CommandTypeDef cmd={0};

    QSPI_WriteEnable();

    cmd.InstructionMode=QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction=CMD_SECTOR_ERASE;

    cmd.AddressMode=QSPI_ADDRESS_1_LINE;
    cmd.AddressSize=QSPI_ADDRESS_24_BITS;
    cmd.Address=addr;

    cmd.DataMode=QSPI_DATA_NONE;

    HAL_QSPI_Command(&hqspi,&cmd,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

    return QSPI_WaitBusy();
}

HAL_StatusTypeDef QSPI_Flash_EraseBlock(uint32_t addr)
{
    QSPI_CommandTypeDef cmd={0};

    QSPI_WriteEnable();

    cmd.InstructionMode=QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction=CMD_BLOCK_ERASE;

    cmd.AddressMode=QSPI_ADDRESS_1_LINE;
    cmd.AddressSize=QSPI_ADDRESS_24_BITS;
    cmd.Address=addr;

    cmd.DataMode=QSPI_DATA_NONE;

    HAL_QSPI_Command(&hqspi,&cmd,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

    return QSPI_WaitBusy();
}

HAL_StatusTypeDef QSPI_Flash_EraseChip(void)
{
    QSPI_CommandTypeDef cmd={0};

    QSPI_WriteEnable();

    cmd.InstructionMode=QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction=CMD_CHIP_ERASE;

    cmd.AddressMode=QSPI_ADDRESS_NONE;
    cmd.DataMode=QSPI_DATA_NONE;

    HAL_QSPI_Command(&hqspi,&cmd,HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

    return QSPI_WaitBusy();
}

HAL_StatusTypeDef QSPI_EnableMemoryMapped(void)
{
    QSPI_CommandTypeDef cmd={0};
    QSPI_MemoryMappedTypeDef cfg={0};

    cmd.InstructionMode=QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction=CMD_FAST_READ_QUAD;

    cmd.AddressMode=QSPI_ADDRESS_4_LINES;
    cmd.AddressSize=QSPI_ADDRESS_24_BITS;

    cmd.DataMode=QSPI_DATA_4_LINES;
    cmd.DummyCycles=6;

    cfg.TimeOutActivation=QSPI_TIMEOUT_COUNTER_DISABLE;

    return HAL_QSPI_MemoryMapped(&hqspi,&cmd,&cfg);
}
