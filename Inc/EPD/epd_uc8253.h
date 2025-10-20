#ifndef __EPD_UC8253_H__
#define __EPD_UC8253_H__

#include <stdint.h>

typedef enum{
    EPD_WHITE, // 白色
    EPD_BLACK, // 黑色
    EPD_RED    // 红色
} EPD_COLOR;

///跟引脚密切相关的，需要根据硬件修改
void EPD_Rest(void); ///RST_N对应的引脚
void EPD_WaitUntilIdle();
///!跟硬件密切相关的，需要根据硬件修改

// 初始化与基本操作
/**
 * @brief EPD_IsOk
 * @return 1 表示良好 0 表示损坏
 */
uint8_t EPD_IsOk(void);
void EPD_Init(void);
void EPD_Sleep(void);
/**
 * @brief 清除屏幕
 * @param 要清屏的颜色
 */
void EPD_Clear(EPD_COLOR color);
void EPD_Done(void);

/**
 * @brief 绘制一个像素颜色
 * @param x
 * @param y
 * @param color
 */
void EPD_InitDrawBuffer(EPD_COLOR color);
void EPD_DrawPixel(uint16_t x, uint16_t y, EPD_COLOR color);
void EPD_ShowBuffer();

// 局部刷新接口
void EPD_DisplayPartial(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);

// 底层 SPI 通信
void EPD_SendCommand(uint8_t cmd);
void EPD_SendData(uint8_t data);
void EPD_SendBuffer(const unsigned char* pBuffer,uint16_t unLength);

/// 获取内部温度
uint8_t EPD_GetInnerTemp();

#endif
