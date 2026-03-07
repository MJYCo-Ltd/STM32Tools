/*
 * SoftSpi.c
 *
 *  Created on: May 7, 2024
 *      Author: yty
 */
#include "SoftSpi.h"
#include "cmsis_os.h"
#include "main.h"

/** \brief 定义SPI引脚操作宏 */
#define SCL_UNDER HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_RESET)
#define SCL_HIGH HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_SET)
#define SDA_UNDER HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_RESET)
#define SDA_HIGH HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_SET)

__attribute__((weak)) void SPI_Write(uint8_t data) {
  uint8_t i;
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
}

__attribute__((weak)) void SPI_WriteBuffer(const uint8_t *pBuffer,
                                           uint16_t unLength) {
  for (uint16_t i = 0; i < unLength; i++) {
    SPI_Write(pBuffer[i]);
  }
}
