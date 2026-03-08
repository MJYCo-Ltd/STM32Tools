#ifndef __YTY_LCD_H
#define __YTY_LCD_H

#include <Display/Display.h>

/**
 * @brief 初始化LCD 根据型号进行初始化
 */
void LCD_Init(void);

/**
 * @brief LCD复位
 */
void LCD_Reset(void);

/**
 * @brief 设置旋转
 * @param m
 */
void LCD_SetRotation(ROTATION m);

/**
 * @brief 设置显存写入窗口区域
 * @param x0,y0 窗口左上角坐标
 * @param x1,y1 窗口右下角坐标
 */
void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * @brief 刷新帧缓冲区数据
 */
void LCD_Refresh();

/**
 * @brief 全屏颜色反显开关
 * @param invert 1=开启反显，0=关闭
 */
void LCD_InvertColors(uint8_t invert);
/**
 * @brief 撕裂效应线开关（用于同步刷新）
 * @param tear 1=开启，0=关闭
 */
void LCD_TearEffect(uint8_t tear);
#endif /// end __YTY_LCD_H
