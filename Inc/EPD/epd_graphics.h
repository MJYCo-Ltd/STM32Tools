#ifndef __EPD_GRAPHICS_H__
#define __EPD_GRAPHICS_H__

#include "epd_uc8253.h"



// 基本绘图函数
void EPD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color);
void EPD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);
void EPD_DrawFilledRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);
void EPD_DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t color);
void EPD_DrawFilledCircle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t color);

// 字符绘制（简单 5x7 字体）
void EPD_DrawChar(uint16_t x, uint16_t y, char c, uint8_t color);
void EPD_DrawString(uint16_t x, uint16_t y, const char *str, uint8_t color);

// 显示帧缓冲区
void EPD_DisplayBuffer(uint8_t *buffer);

#endif
