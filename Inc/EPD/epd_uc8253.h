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
/**
 * @brief 刷新屏幕
 * @attention EPD_Clear、EPD_ShowBuffer之后
 * 如果不调用该函数将无法将更改刷新到屏幕上
 * @attention 此函数需要在 EPD_PowerOn()之后才能生效
 */
void EPD_Update(void);

/**
 * @brief 初始化内存的buffer
 */
void EPD_InitDrawBuffer(EPD_COLOR color);
void EPD_DrawPixel(const EPD_Pixel* pPixel);
void EPD_ShowBuffer(void);

/**
 * @brief 局刷接口，内部会自动更新，不需要调用EPD_Update了
 * @param x 像素位置
 * @param y 像素位置
 * @param w 像素宽度
 * @param h 像素高度
 * @param data 像素数据
 */
void EPD_DisplayPartial(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const EPD_Pixel *data);

#endif
