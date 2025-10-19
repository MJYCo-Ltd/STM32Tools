#include "main.h"
#include "cmsis_os.h"

#define EPD_WIDTH  240
#define EPD_HEIGHT 416
#define EPD_BUFFER_SIZE 12480

#define SELECT_EPD HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)
#define UNSELECT_EPD HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)

#define EPD_CMD HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET)
#define EPD_DATA HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET)

#define EPD_ISBUSY (HAL_GPIO_ReadPin(BUSY_GPIO_Port, BUSY_Pin) == GPIO_PIN_RESET)

#define SCL_UNDER HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_RESET)
#define SCL_HIGH HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_SET)

#define SDA_UNDER HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_RESET)
#define SDA_HIGH HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_SET)

///用引脚模拟SPI发送数据
void SPI_Write(uint8_t dat)
{
    uint8_t i;
    SELECT_EPD;
    for(i=0;i<8;i++)
    {
        SCL_UNDER;
        if(dat&0x80)
        {
            SDA_HIGH;
        }
        else
        {
            SDA_UNDER;
        }
        SCL_HIGH;
        dat<<=1;
    }
    UNSELECT_EPD;
}

void SPI_WriteBuffer(const unsigned char* pBuffer,uint16_t unLength)
{
}

///根据手册拉低RES，200微秒以上
void EPD_Rest()
{
    HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);
    osDelay(10);
    HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
    osDelay(10);
}

void EPD_WaitUntilIdle()
{
  while(EPD_ISBUSY)
{
osDelay(1);
}
}
