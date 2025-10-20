#include "cmsis_os.h"
#include "main.h"

#define EPD_WIDTH 240
#define EPD_HEIGHT 416
#define EPD_BUFFER_SIZE 12480

#define SELECT_EPD HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)
#define UNSELECT_EPD HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)

#define EPD_CMD HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET)
#define EPD_DATA HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET)

#ifdef NO_SPI_
/// 通过引脚模拟 SPI
#define SCL_UNDER HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_RESET)
#define SCL_HIGH HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_SET)
#define SDA_UNDER HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_RESET)
#define SDA_HIGH HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_SET)
///!通过引脚模拟 SPI
#else
extern SPI_HandleTypeDef hspi1;
#endif // NO_SPI_

#ifdef NO_SPI_
/// 用引脚模拟SPI发送数据
__attribute__((weak)) void SPI_Write(uint8_t data) {
  uint8_t i;
  SELECT_EPD;
  for (i = 0; i < 8; i++) {
    SCL_UNDER;
    if (dat & 0x80) {
      SDA_HIGH;
    } else {
      SDA_UNDER;
    }
    SCL_HIGH;
    dat <<= 1;
  }
  UNSELECT_EPD;
}
#else
__attribute__((weak)) void SPI_Write(uint8_t data) {
  HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}
#endif // NO_SPI_
__attribute__((weak)) void SPI_WriteBuffer(const unsigned char *pBuffer,
                                           uint16_t unLength) {
  HAL_SPI_Transmit(&hspi1, pBuffer, unLength, HAL_MAX_DELAY);
}

/// 根据手册RST_N拉低50us以上
__attribute__((weak)) void EPD_Rest() {
  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);
  osDelay(10);
  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
  osDelay(10);
}

/// 根据手册BUSY_N 高电平为空闲
__attribute__((weak)) void EPD_WaitUntilIdle() {
  while (HAL_GPIO_ReadPin(BUSY_GPIO_Port, BUSY_Pin) == GPIO_PIN_RESET) {
    osDelay(1);
  }
}

/// 获取内部温度
__attribute__((weak)) uint8_t EPD_GetInnerTemp() {
  EPD_SendCommand(EPD_CMD_POWER_ON);
  EPD_WaitUntilIdle();
  EPD_SendCommand(EPD_CMD_TEMPERATURE_SENSOR_CALIBRATION);
  EPD_WaitUntilIdle();

  uint8_t data;
  HAL_SPI_Receive(&hspi1, &data, 1, HAL_MAX_DELAY);

  EPD_Sleep();
  return (data);
}

/// 检查面板玻璃
__attribute__((weak)) uint8_t EPD_IsOk() {
  EPD_SendCommand(EPD_CMD_PANEL_GLASS_CHECK);
  EPD_WaitUntilIdle();

  uint8_t data;
  HAL_SPI_Receive(&hspi1, &data, 1, HAL_MAX_DELAY);
  return (data);
}
