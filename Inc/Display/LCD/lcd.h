#ifndef __YTY_LCD_H
#define __YTY_LCD_H

#include <Display/Display.h>

/* @brief 初始化LCD 根据型号进行初始化
 */
void LCD_Init(void);

/* @brief LCD复位
 */
void LCD_Reset(void);

void LCD_SetRotation(ROTATION m);

void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void LCD_Clear(uint16_t color);

void LCD_TearEffect(uint8_t tear);
void lcd_st7789_draw_buffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                            uint16_t *buf);
#endif /// end __YTY_LCD_H
