#include "cmsis_os.h"
#include "main.h"

#define EPD_WIDTH 240
#define EPD_HEIGHT 416
#define EPD_BUFFER_SIZE 12480

#define SPI_SELECT HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)
#define SPI_UNSELECT HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)

#define SPI_SEND_CMD HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET)
#define SPI_SEND_DATA HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET)

#define DISPLAY_SPI_PORT hspi1
extern SPI_HandleTypeDef DISPLAY_SPI_PORT;

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
