#include "cmsis_os.h"
#include "main.h"

#define EPD_WIDTH 240
#define EPD_HEIGHT 416
#define EPD_BUFFER_SIZE 12480

#define SELECT_EPD HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)
#define UNSELECT_EPD HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)

#define EPD_CMD HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET)
#define EPD_DATA HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET)

#define EPD_SPI hspi1
extern SPI_HandleTypeDef EPD_SPI;

__attribute__((weak)) void SPI_WriteBuffer(const unsigned char *pBuffer,
                                           uint16_t unLength) {
  HAL_SPI_Transmit(&EPD_SPI, pBuffer, unLength, HAL_MAX_DELAY);
}

/// 根据手册RST_N拉低50us以上
__attribute__((weak)) void EPD_Rest(void) {
  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);
  osDelay(10);
  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
  osDelay(10);
}

/// 根据手册BUSY_N 高电平为空闲
__attribute__((weak)) void EPD_WaitUntilIdle(void) {
  while (HAL_GPIO_ReadPin(BUSY_GPIO_Port, BUSY_Pin) == GPIO_PIN_RESET) {
    osDelay(1);
  }
}
