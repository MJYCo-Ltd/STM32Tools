#include "main.h"

/**
 * @brief 选择使用的硬件 SPI 端口
 */
#define ST7789_SPI_PORT hspi4
extern SPI_HandleTypeDef ST7789_SPI_PORT;

/* 是否使用 DMA 传输（需 MCU 有足够 RAM） */
// #define USE_DMA
#define USE_BUFFER
/* 若不需要 CS 片选控制，则定义此项 */
#define CFG_NO_CS
/* 若不需要 RST 复位引脚，则定义此项（将使用软件复位） */
#define CFG_NO_REST

#ifndef CFG_NO_REST
#define ST7789_RST_PORT LCD_RST_GPIO_Port
#define ST7789_RST_PIN LCD_RST_Pin
#endif

/* 数据/指令选择引脚（DC：低电平=指令，高电平=数据） */
#define ST7789_DC_PORT LCD_DC_GPIO_Port
#define ST7789_DC_PIN LCD_DC_Pin

#ifndef CFG_NO_CS
#define ST7789_CS_PORT ST7789_CS_GPIO_Port
#define ST7789_CS_PIN ST7789_CS_Pin
#endif

/* 背光控制：低电平关闭，高电平开启 */
#define LCD_Backlight_OFF                                                      \
  HAL_GPIO_WritePin(LCD_BlackLight_GPIO_Port, LCD_BlackLight_Pin,              \
                    GPIO_PIN_RESET)
#define LCD_Backlight_ON                                                       \
  HAL_GPIO_WritePin(LCD_BlackLight_GPIO_Port, LCD_BlackLight_Pin, GPIO_PIN_SET)

/* DC 引脚：低电平=传输指令，高电平=传输数据 */
#define LCD_SEND_CMD                                                           \
  HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_RESET)
#define LCD_SEND_DATA                                                          \
  HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_SET)
/*
 * 屏幕尺寸选择（三选一）
 * 135x240(0.96寸) / 240x240(1.3寸) / 170x320(1.9寸)
 * X_SHIFT、Y_SHIFT 用于适配不同分辨率屏幕的显存偏移
 */

// #define USING_135X240
#define USING_240X240
// #define USING_170X320

/* 显示方向选择 (0-3) */
// #define ST7789_ROTATION 0
// #define ST7789_ROTATION 1
#define ST7789_ROTATION 2 /* 240x240 常用竖屏 */
// #define ST7789_ROTATION 3

#ifdef USING_135X240

#if ST7789_ROTATION == 0
#define ST7789_WIDTH 135
#define ST7789_HEIGHT 240
#define X_SHIFT 53
#define Y_SHIFT 40
#endif

#if ST7789_ROTATION == 1
#define ST7789_WIDTH 240
#define ST7789_HEIGHT 135
#define X_SHIFT 40
#define Y_SHIFT 52
#endif

#if ST7789_ROTATION == 2
#define ST7789_WIDTH 135
#define ST7789_HEIGHT 240
#define X_SHIFT 52
#define Y_SHIFT 40
#endif

#if ST7789_ROTATION == 3
#define ST7789_WIDTH 240
#define ST7789_HEIGHT 135
#define X_SHIFT 40
#define Y_SHIFT 53
#endif

#endif

#ifdef USING_240X240

#define ST7789_WIDTH 240
#define ST7789_HEIGHT 240

#if ST7789_ROTATION == 0
#define X_SHIFT 0
#define Y_SHIFT 80
#elif ST7789_ROTATION == 1
#define X_SHIFT 80
#define Y_SHIFT 0
#elif ST7789_ROTATION == 2
#define X_SHIFT 0
#define Y_SHIFT 0
#elif ST7789_ROTATION == 3
#define X_SHIFT 0
#define Y_SHIFT 0
#endif

#endif

#ifdef USING_170X320

#if ST7789_ROTATION == 0
#define ST7789_WIDTH 170
#define ST7789_HEIGHT 320
#define X_SHIFT 35
#define Y_SHIFT 0
#endif

#if ST7789_ROTATION == 1
#define ST7789_WIDTH 320
#define ST7789_HEIGHT 170
#define X_SHIFT 0
#define Y_SHIFT 35
#endif

#if ST7789_ROTATION == 2
#define ST7789_WIDTH 170
#define ST7789_HEIGHT 320
#define X_SHIFT 35
#define Y_SHIFT 0
#endif

#if ST7789_ROTATION == 3
#define ST7789_WIDTH 320
#define ST7789_HEIGHT 170
#define X_SHIFT 0
#define Y_SHIFT 35
#endif

#endif

#ifdef USE_BUFFER
#include <string.h>
#define LCD_BUFFER_SIZE ST7789_WIDTH * ST7789_HEIGHT * 2
__attribute__((section(".dma_buffer"), aligned(32))) uint8_t lcd_buffer[LCD_BUFFER_SIZE];
#endif
/**
 * @brief 预定义颜色（RGB565 格式）
 *        如需自定义颜色，可使用 RGB565 格式，如 0xRRGGBB 转换
 */
#define LCD_WHITE 0xFFFF
#define LCD_BLACK 0x0000
#define LCD_BLUE 0x001F
#define LCD_RED 0xF800
#define LCD_MAGENTA 0xF81F
#define LCD_GREEN 0x07E0
#define LCD_CYAN 0x7FFF
#define LCD_YELLOW 0xFFE0
#define LCD_GRAY 0X8430
#define LCD_BRED 0XF81F
#define LCD_GRED 0XFFE0
#define LCD_GBLUE 0X07FF
#define LCD_BROWN 0XBC40
#define LCD_BRRED 0XFC07
#define LCD_DARKBLUE 0X01CF
#define LCD_LIGHTBLUE 0X7D7C
#define LCD_GRAYBLUE 0X5458

#define LCD_LIGHTGREEN 0X841F
#define LCD_LGRAY 0XC618
#define LCD_LGRAYBLUE 0XA651
#define LCD_LBBLUE 0X2B12

/* ST7789 控制器寄存器及常量定义 */
#define ST7789_NOP 0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID 0x04
#define ST7789_RDDST 0x09

#define ST7789_SLPIN 0x10
#define ST7789_SLPOUT 0x11
#define ST7789_PTLON 0x12
#define ST7789_NORON 0x13

#define ST7789_INVOFF 0x20
#define ST7789_INVON 0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON 0x29
#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C
#define ST7789_RAMRD 0x2E

#define ST7789_PTLAR 0x30
#define ST7789_COLMOD 0x3A
#define ST7789_MADCTL 0x36

/// MADCTL 方向值：与 LCD_SetDirection 对应
#define ST7789_MADCTL_VERTICAL 0x00   /* Direction_V 竖屏 */
#define ST7789_MADCTL_HORIZONTAL 0x70 /* Direction_H 横屏 */
#define ST7789_MADCTL_H_FLIP 0xA0     /* Direction_H_Flip 横屏翻转 */
#define ST7789_MADCTL_V_FLIP 0xC0     /* Direction_V_Flip 竖屏翻转 */

#define ST7789_RDID1 0xDA
#define ST7789_RDID2 0xDB
#define ST7789_RDID3 0xDC
#define ST7789_RDID4 0xDD

/* 像素格式：0x05=16位RGB565，0x06=18位RGB666 */
#define ST7789_COLOR_MODE_16bit 0x05
#define ST7789_COLOR_MODE_18bit 0x06

#ifndef CFG_NO_REST
#define ST7789_RST_Clr()                                                       \
  HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_RESET)
#define ST7789_RST_Set()                                                       \
  HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_SET)
#endif

#ifndef CFG_NO_CS
#define ST7789_Select()                                                        \
  HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_RESET)
#define ST7789_UnSelect()                                                      \
  HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_SET)
#else
#define ST7789_Select()
#define ST7789_UnSelect()
#endif

#ifndef ST7789_ROTATION
#error 请至少选择一种显示方向 (ST7789_ROTATION 0-3)
#endif
