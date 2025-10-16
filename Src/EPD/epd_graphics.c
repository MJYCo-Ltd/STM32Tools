#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "epd_graphics.h"

uint8_t epd_frame[12480];

// ========== 绘制像素 ==========
void EPD_DrawPixel(uint16_t x, uint16_t y, uint8_t color) {
    if (x >= EPD_WIDTH || y >= EPD_HEIGHT) return;

    uint32_t index = x + y * EPD_WIDTH;
    uint32_t byte_index = index / 8;
    uint8_t bit_mask = 0x80 >> (index % 8);

    if (color == EPD_COLOR_BLACK)
        epd_frame[byte_index] &= ~bit_mask;
    else
        epd_frame[byte_index] |= bit_mask;
}

// ========== 直线（Bresenham） ==========
void EPD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color) {
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (1) {
        EPD_DrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// ========== 矩形 ==========
void EPD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color) {
    EPD_DrawLine(x, y, x + w - 1, y, color);
    EPD_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    EPD_DrawLine(x, y, x, y + h - 1, color);
    EPD_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

// ========== 填充矩形 ==========
void EPD_DrawFilledRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color) {
    for (uint16_t i = 0; i < h; i++)
        for (uint16_t j = 0; j < w; j++)
            EPD_DrawPixel(x + j, y + i, color);
}

// ========== 圆形（中点算法） ==========
void EPD_DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t color) {
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 0;

    while (x >= y) {
        EPD_DrawPixel(x0 + x, y0 + y, color);
        EPD_DrawPixel(x0 + y, y0 + x, color);
        EPD_DrawPixel(x0 - y, y0 + x, color);
        EPD_DrawPixel(x0 - x, y0 + y, color);
        EPD_DrawPixel(x0 - x, y0 - y, color);
        EPD_DrawPixel(x0 - y, y0 - x, color);
        EPD_DrawPixel(x0 + y, y0 - x, color);
        EPD_DrawPixel(x0 + x, y0 - y, color);

        y++;
        if (err <= 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x + 1); }
    }
}

// ========== 填充圆形 ==========
void EPD_DrawFilledCircle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t color) {
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 0;

    while (x >= y) {
        EPD_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
        EPD_DrawLine(x0 - y, y0 + x, x0 + y, y0 + x, color);
        EPD_DrawLine(x0 - x, y0 - y, x0 + x, y0 - y, color);
        EPD_DrawLine(x0 - y, y0 - x, x0 + y, y0 - x, color);

        y++;
        if (err <= 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x + 1); }
    }
}

// ========== 5x7 字体 ==========
static const uint8_t font5x7[] = {
    // 每个字符5x7，ASCII 32~127
    0x00,0x00,0x00,0x00,0x00, // ' '
    0x00,0x00,0x5F,0x00,0x00, // '!'
    0x00,0x07,0x00,0x07,0x00, // '"'
    0x14,0x7F,0x14,0x7F,0x14, // '#'
    0x24,0x2A,0x7F,0x2A,0x12, // '$'
    // ...可自行补充（此处省略部分）
};

// 绘制单个字符
void EPD_DrawChar(uint16_t x, uint16_t y, char c, uint8_t color) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t *chr = &font5x7[(c - 32) * 5];

    for (uint8_t i = 0; i < 5; i++) {
        uint8_t line = chr[i];
        for (uint8_t j = 0; j < 7; j++) {
            if (line & 0x01)
                EPD_DrawPixel(x + i, y + j, color);
            else
                EPD_DrawPixel(x + i, y + j, !color);
            line >>= 1;
        }
    }
}

// 绘制字符串
void EPD_DrawString(uint16_t x, uint16_t y, const char *str, uint8_t color) {
    while (*str) {
        EPD_DrawChar(x, y, *str, color);
        x += 6; // 字间距
        str++;
    }
}

#include "Auxiliary.h"
//extern SPI_HandleTypeDef hspi1;
// 显示缓冲区
void EPD_DisplayBuffer(uint8_t *buffer) {
    uint32_t total = (EPD_WIDTH * EPD_HEIGHT) / 8;
	
	if(total == 12480)
	{
		SendDebugInfo("OK", 2);
	}
	EPD_SendCommand(0x10);
	EPD_SendBuffer(buffer,total);
	EPD_WaitUntilIdle();
    EPD_SendCommand(0x13);
	EPD_SendBuffer(buffer,total);
	EPD_WaitUntilIdle();
	EPD_SendCommand(0x11);
	uint8_t value=0;
//	if(HAL_OK == HAL_SPI_Receive(&hspi1, &value, 1, 1))
//	{
//		if(0 == value){
//			SendDebugInfo("0", 2);
//		}
//		if(1 == value){
//			SendDebugInfo("1", 2);
//		}
//	}
    EPD_DisplayFrame();
}
