/*
 ******************************************************************************
 * @file           : Display.h
 * @brief          : 此文件为显示的基础头文件，包含一些基本的类型定义
 ******************************************************************************
 *
 *  Created on: May 7, 2026
 *      Author: yty
 */
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdint.h>
typedef struct {
    uint8_t uRed;
    uint8_t uGreen;
    uint8_t uBlue;
} COLOR; /// 颜色值

/**
 * @brief 判断两个颜色是否相等
 * @param a 颜色a
 * @param b 颜色b
 * @return 1表示相等 0表示不相等
 */
static inline int COLOR_EQUAL(COLOR a, COLOR b) {
    return (a.uRed == b.uRed) && (a.uGreen == b.uGreen) && (a.uBlue == b.uBlue);
}

/**
 * @brief 将COLOR类型转换为RGB565格式
 * @param color 颜色值
 * @return RGB565格式的颜色值
 */
static inline uint16_t ColorToRGB565(COLOR color) {
    // 红色：右移 3 位，再左移 11 位到高位
    // 绿色：右移 2 位，再左移 5 位到中间
    // 蓝色：右移 3 位，放在低位
    return ((uint16_t)(color.uRed & 0xF8) << 8) |
           ((uint16_t)(color.uGreen & 0xFC) << 3) |
           ((uint16_t)(color.uBlue >> 3));
}

typedef struct {
    uint16_t x;
    uint16_t y;
    COLOR color;
} Pixel; /// 像素值

typedef enum {
    NO_ROTATION = 0,
    ROTATION_90,
    ROTATION_180,
    ROTATION_270
} ROTATION; /// 显示旋转
#endif
