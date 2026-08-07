#include "main.h"

/**
 * @brief ST7305 硬件 SPI 及面板配置
 *
 * 参数适用于 FOCUS DISPLAY FD042MN-ZF21-H06-B：300x400、Normally White、
 * 4-line SPI、无背光。模拟电压与时序采用 ST7305 300x400 面板参考序列。
 */
#define DISPLAY_SPI_PORT hspi1
extern SPI_HandleTypeDef DISPLAY_SPI_PORT;

/* ST7305 的像素不能独立写入，驱动始终使用 1bpp 帧缓冲。 */
#define USE_BUFFER
/* 如 SPI 已配置 TX DMA，可取消下一行注释。 */
// #define USE_DMA

/* 此模组的 CS 和 RESET 均为独立引脚。 */

#ifndef CFG_NO_REST
#define ST7305_RST_PORT LCD_RST_GPIO_Port
#define ST7305_RST_PIN  LCD_RST_Pin
#endif

#define ST7305_DC_PORT LCD_DC_GPIO_Port
#define ST7305_DC_PIN  LCD_DC_Pin
#define ST_DISPLAY_DC_PORT ST7305_DC_PORT
#define ST_DISPLAY_DC_PIN  ST7305_DC_PIN

#ifndef CFG_NO_CS
#define ST7305_CS_PORT LCD_CS_GPIO_Port
#define ST7305_CS_PIN  LCD_CS_Pin
#define ST_DISPLAY_CS_PORT ST7305_CS_PORT
#define ST_DISPLAY_CS_PIN  ST7305_CS_PIN
#endif

#define SPI_SEND_CMD                                                           \
  HAL_GPIO_WritePin(ST7305_DC_PORT, ST7305_DC_PIN, GPIO_PIN_RESET)
#define SPI_SEND_DATA                                                          \
  HAL_GPIO_WritePin(ST7305_DC_PORT, ST7305_DC_PIN, GPIO_PIN_SET)

#ifndef CFG_NO_CS
#define SPI_SELECT                                                             \
  HAL_GPIO_WritePin(ST7305_CS_PORT, ST7305_CS_PIN, GPIO_PIN_RESET)
#define SPI_UNSELECT                                                           \
  HAL_GPIO_WritePin(ST7305_CS_PORT, ST7305_CS_PIN, GPIO_PIN_SET)
#else
#define SPI_SELECT
#define SPI_UNSELECT
#endif

#ifndef CFG_NO_REST
#define ST7305_RST_Clr()                                                        \
  HAL_GPIO_WritePin(ST7305_RST_PORT, ST7305_RST_PIN, GPIO_PIN_RESET)
#define ST7305_RST_Set()                                                        \
  HAL_GPIO_WritePin(ST7305_RST_PORT, ST7305_RST_PIN, GPIO_PIN_SET)
#endif

/* FD042MN-ZF21-H06-B：300x400，横向恰好为 25 个 12 像素 GRAM 单元。 */
#define ST7305_WIDTH          300U
#define ST7305_HEIGHT         400U
#define ST7305_COLUMN_OFFSET  0x01U
#define ST7305_COLUMN_END     0x2AU
#define ST7305_ROW_OFFSET     0x00U
#define ST7305_GATE_LINES     0x64U
#define ST7305_MADCTL_VALUE   0x48U
/* 模组保持原生 300x400 初始化，绘图坐标向左旋转 90 度为 400x300。 */
#define ST7305_DEFAULT_ROTATION ROTATION_270

/* 厂商推荐的模拟电压和时序参数。 */
#define ST7305_NVM_LOAD_0     0x13U
#define ST7305_NVM_LOAD_1     0x02U
#define ST7305_GATE_VOLTAGE_0 0x12U
#define ST7305_GATE_VOLTAGE_1 0x0AU
#define ST7305_OSC_VALUE      0xA6U
#define ST7305_FRAME_RATE     0x12U
#define ST7305_VSHP_VALUES    {0x3CU, 0x3EU, 0x3CU, 0x3CU}
#define ST7305_VSLP_VALUES    {0x23U, 0x21U, 0x23U, 0x23U}
#define ST7305_VSHN_VALUES    {0x5AU, 0x5CU, 0x5AU, 0x5AU}
#define ST7305_VSLN_VALUES    {0x37U, 0x35U, 0x37U, 0x37U}
#define ST7305_HPM_EQ_VALUES                                                   \
  {0xE5U, 0xF6U, 0x17U, 0x77U, 0x77U, 0x77U, 0x77U, 0x77U, 0x77U, 0x71U}
#define ST7305_LPM_EQ_VALUES                                                   \
  {0x05U, 0x46U, 0x77U, 0x77U, 0x77U, 0x77U, 0x76U, 0x45U}
#define ST7305_GATE_TIMING_VALUES {0x32U, 0x03U, 0x1FU}
