/*
 ******************************************************************************
 * @file           : Graphics.h
 * @brief          :
 * 此文件为显示的基础头文件，包含一些基本的绘图函数和字符绘制函数
 ******************************************************************************
 *
 *  Created on: May 7, 2026
 *      Author: yty
 */
#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include "Display/Display.h"

void DrawPixel(const Pixel *pPixel);

/// 基本绘图函数
void DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, COLOR color);
void DrawHLine(uint16_t x0, uint16_t y0, uint16_t x1, COLOR color);
void DrawVLine(uint16_t x0, uint16_t y0, uint16_t y1, COLOR color);

void DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, COLOR color);
void DrawFilledRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    COLOR color);
void DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, COLOR color);
void DrawFilledCircle(uint16_t x0, uint16_t y0, uint16_t r, COLOR color);
void DrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  uint16_t x2, uint16_t y2, COLOR color);
void DrawFilledTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                        uint16_t x2, uint16_t y2, COLOR color);
/// 字符绘制（简单 5x7 字体）
void DrawChar(uint16_t x, uint16_t y, char c, COLOR color);
void DrawString(uint16_t x, uint16_t y, const char *str, COLOR color);

#endif
