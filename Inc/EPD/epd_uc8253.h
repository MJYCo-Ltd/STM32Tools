#ifndef __EPD_UC8253_H__
#define __EPD_UC8253_H__

#include <stdint.h>

typedef enum{
    EPD_TWO_COLOR,  ///黑白双色模式
    EPD_THREE_COLOR ///黑白红三色模式
}EPD_MODEL;///墨水屏模式

typedef enum{
    EPD_WHITE, /// 白色
    EPD_BLACK, /// 黑色
    EPD_RED    /// 红色
} EPD_COLOR;

typedef struct
{
    uint16_t x;
    uint16_t y;
    EPD_COLOR color;
}EPD_Pixel;///墨水屏的像素值

///DeepSleep之后所有寄存器配置（LUT、Panel Setting、分辨率等）都会被清空
///需要调用Rest才能唤醒
void EPD_Rest(void);
void EPD_DeepSleep(void);

/// 开/关机
/// 不开关机写入数据不能刷新
void EPD_PowerOn(void);
void EPD_PowerOff(void);

/**
 * @brief 使用出厂模式更新模式
 * @param model 模式需要跟板子匹配，不能把三色屏强制设置EPD_TWO_COLOR模式
 * @param fast 0表示默认刷新 非0值表示快速刷新
 */
void EPD_Init(EPD_MODEL model, uint8_t fastFresh);

/**
 * @brief EPD_IsOk
 * @return 1 表示良好 0 表示损坏
 */
uint8_t EPD_IsOk(void);
/// 获取内部温度
uint8_t EPD_GetInnerTemp(void);

/**
 * @brief 清除屏幕
 * @param 要清屏的颜色
 */
void EPD_Clear(EPD_COLOR color);
void EPD_Update(void);

/**
 * @brief 绘制一个像素颜色
 * @param x
 * @param y
 * @param color
 */
void EPD_InitDrawBuffer(EPD_COLOR color);
void EPD_DrawPixel(const EPD_Pixel* pPixel);
void EPD_ShowBuffer(void);

// 局部刷新接口
void EPD_DisplayPartial(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);

// 底层 SPI 通信
void EPD_SendCommand(uint8_t cmd);
void EPD_SendData(uint8_t data);
void EPD_SendBuffer(const unsigned char* pBuffer,uint16_t unLength);

#endif
