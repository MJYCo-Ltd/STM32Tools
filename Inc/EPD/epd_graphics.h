#ifndef __EPD_GRAPHICS_H__
#define __EPD_GRAPHICS_H__

#include "EPD/epd_uc8253.h"



// 基本绘图函数
void EPD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, EPD_COLOR color);
void EPD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, EPD_COLOR color);
void EPD_DrawFilledRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, EPD_COLOR color);
void EPD_DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, EPD_COLOR color);
void EPD_DrawFilledCircle(uint16_t x0, uint16_t y0, uint16_t r, EPD_COLOR color);

// 字符绘制（简单 5x7 字体）
void EPD_DrawChar(uint16_t x, uint16_t y, char c, EPD_COLOR color);
void EPD_DrawString(uint16_t x, uint16_t y, const char *str, EPD_COLOR color);

#endif
