#ifndef __EPD_UC8253_H__
#define __EPD_UC8253_H__

#include "stm32f1xx_hal.h"

#define EPD_WIDTH  240
#define EPD_HEIGHT 416

#define EPD_COLOR_WHITE 0xFF
#define EPD_COLOR_BLACK 0x00

// 初始化与基本操作
void EPD_Init(void);
void EPD_Wakeup(void);
void EPD_Clear(void);
void EPD_DisplayFrame(void);
void EPD_Sleep(void);

// 局部刷新接口
void EPD_DisplayPartial(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);

// 底层 SPI 通信
void EPD_SendCommand(uint8_t cmd);
void EPD_SendData(uint8_t data);
void EPD_SendBuffer(const unsigned char* pBuffer,uint16_t unLength);
void EPD_WaitUntilIdle(void);
void EPD_Display(const uint8_t *image);
void EPD_Display_Clear(void);

#endif
