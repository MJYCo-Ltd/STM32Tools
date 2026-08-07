#include <cmsis_os.h>
#include <Common.h>
#include <Display/LCD/lcd.h>
#include <Display/LCD/lcd_st7789_config.h>
#include <Display/SPIDisplay.h>

static const uint8_t s_st7789_madctl[] = {ST7789_MADCTL_VERTICAL};
static const uint8_t s_st7789_colmod[] = {ST7789_COLOR_MODE_16bit};
static const uint8_t s_st7789_porch[] = {0x0CU, 0x0CU, 0x00U, 0x33U, 0x33U};
static const uint8_t s_st7789_gate[] = {0x35U};
static const uint8_t s_st7789_vcom[] = {0x19U};
static const uint8_t s_st7789_lcm[] = {0x2CU};
static const uint8_t s_st7789_vdv_vrh[] = {0x01U};
static const uint8_t s_st7789_vrh[] = {0x12U};
static const uint8_t s_st7789_vdv[] = {0x20U};
static const uint8_t s_st7789_frame_rate[] = {0x0FU};
static const uint8_t s_st7789_power[] = {0xA4U, 0xA1U};
static const uint8_t s_st7789_gamma_positive[] = {
    0xD0U, 0x04U, 0x0DU, 0x11U, 0x13U, 0x2BU, 0x3FU,
    0x54U, 0x4CU, 0x18U, 0x0DU, 0x0BU, 0x1FU, 0x23U};
static const uint8_t s_st7789_gamma_negative[] = {
    0xD0U, 0x04U, 0x0CU, 0x11U, 0x13U, 0x2CU, 0x3FU,
    0x44U, 0x51U, 0x2FU, 0x1FU, 0x1FU, 0x20U, 0x23U};

static const SPI_DisplayCommand s_st7789_init_sequence[] = {
    {ST7789_MADCTL, s_st7789_madctl, sizeof(s_st7789_madctl), 0U},
    {ST7789_COLMOD, s_st7789_colmod, sizeof(s_st7789_colmod), 0U},
    {0xB2U, s_st7789_porch, sizeof(s_st7789_porch), 0U},
    {0xB7U, s_st7789_gate, sizeof(s_st7789_gate), 0U},
    {0xBBU, s_st7789_vcom, sizeof(s_st7789_vcom), 0U},
    {0xC0U, s_st7789_lcm, sizeof(s_st7789_lcm), 0U},
    {0xC2U, s_st7789_vdv_vrh, sizeof(s_st7789_vdv_vrh), 0U},
    {0xC3U, s_st7789_vrh, sizeof(s_st7789_vrh), 0U},
    {0xC4U, s_st7789_vdv, sizeof(s_st7789_vdv), 0U},
    {0xC6U, s_st7789_frame_rate, sizeof(s_st7789_frame_rate), 0U},
    {0xD0U, s_st7789_power, sizeof(s_st7789_power), 0U},
    {0xE0U, s_st7789_gamma_positive, sizeof(s_st7789_gamma_positive), 0U},
    {0xE1U, s_st7789_gamma_negative, sizeof(s_st7789_gamma_negative), 0U},
    {ST7789_INVON, NULL, 0U, 0U},
    {ST7789_SLPOUT, NULL, 0U, 120U},
    {ST7789_DISPON, NULL, 0U, 50U}};

static void ST7789_Delay(uint32_t delay_ms) { osDelay(delay_ms); }

/// 设置显示旋转方向
void LCD_SetRotation(ROTATION m) {
  SPI_SendCommand(ST7789_MADCTL);
  switch (m) {
  case NO_ROTATION:
    SPI_SendData(ST7789_MADCTL_VERTICAL); /* 0x00 竖屏 */
    break;
  case ROTATION_90:
    SPI_SendData(ST7789_MADCTL_HORIZONTAL); /* 0x70 横屏 */
    break;
  case ROTATION_180:
    SPI_SendData(ST7789_MADCTL_V_FLIP); /* 0xC0 竖屏翻转 */
    break;
  case ROTATION_270:
    SPI_SendData(ST7789_MADCTL_H_FLIP); /* 0xA0 横屏翻转 */
    break;
  default:
    break;
  }
}

/// 设置显存写入窗口区域
void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
  uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;

  /* Column Address set */
  SPI_SendCommand(ST7789_CASET);
  {
    uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
    SPI_SendBuffer(data, sizeof(data));
  }

  /* Row Address set */
  SPI_SendCommand(ST7789_RASET);
  {
    uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
    SPI_SendBuffer(data, sizeof(data));
  }
  /* Write to RAM */
  SPI_SendCommand(ST7789_RAMWR);
}

/// 初始化 ST7789 控制器
void LCD_Init(void) {
#ifdef USE_BUFFER
  memset(lcd_buffer, LCD_GRAYBLUE, sizeof(lcd_buffer));
#endif

#ifndef CFG_NO_REST
  osDelay(10U);
  ST7789_RST_Clr();
  osDelay(10U);
  ST7789_RST_Set();
  osDelay(20U);
#else
  SPI_SendCommand(ST7789_SWRESET);
#endif
  osDelay(1000U);
  (void)SPI_DisplayRunSequence(
      SPI_GetDisplayBus(), s_st7789_init_sequence,
      sizeof(s_st7789_init_sequence) / sizeof(s_st7789_init_sequence[0]),
      ST7789_Delay);
#ifdef USE_BUFFER
  LCD_Refresh();
#endif
  LCD_Backlight_ON;
}

/// 刷新帧缓冲区数据
void LCD_Refresh() {
  LCD_SetAddressWindow(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);

#ifdef USE_BUFFER
    SPI_SendBuffer(lcd_buffer, sizeof(lcd_buffer));
#else
  uint16_t j;
  for (i = 0; i < ST7789_WIDTH; i++)
    for (j = 0; j < ST7789_HEIGHT; j++) {
      uint8_t data[] = {LCD_BLACK >> 8, LCD_BLACK & 0xFF};
      SPI_SendBuffer(data, sizeof(data));
    }
#endif
}

/// 实现Display的绘制像素的方法
void DrawPixel(const Pixel *pPixel) {
  uint8_t colorData[2];

  if ((pPixel->x < 0) || (pPixel->x >= ST7789_WIDTH) || (pPixel->y < 0) ||
      (pPixel->y >= ST7789_HEIGHT))
    return;
#ifdef USE_BUFFER
  WriteBE16(colorData, ColorToRGB565(pPixel->color));
  memcpy(lcd_buffer + (pPixel->x + pPixel->y * ST7789_WIDTH) * 2U,
         colorData, sizeof(colorData));
#else
  LCD_SetAddressWindow(pPixel->x, pPixel->y, pPixel->x, pPixel->y);
  WriteBE16(colorData, ColorToRGB565(pPixel->color));
  SPI_SendBuffer(colorData, sizeof(colorData));
#endif
}

/// 全屏颜色反显开关
void LCD_InvertColors(uint8_t invert) {
  SPI_SendCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
}

///撕裂效应线开关
void LCD_TearEffect(uint8_t tear) {
  SPI_SendCommand(tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */);
}
