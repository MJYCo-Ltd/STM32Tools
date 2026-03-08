#include <cmsis_os.h>
#include <Common.h>
#include <Display/LCD/lcd.h>
#include <Display/LCD/lcd_user.c>
#include <Display/SPIDisplay.h>

HALF_WORD_DATA tmp_color;
uint8_t uColorData[2];

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
  memset(lcd_buffer,LCD_GRAYBLUE, sizeof(lcd_buffer));
#endif

  /* 复位：硬件复位或软件复位，完成后需等待至少 5ms 才能发送指令 */
#ifndef CFG_NO_REST
  osDelay(10);
  ST7789_RST_Clr();
  osDelay(10);
  ST7789_RST_Set();
  osDelay(20);
#else
   SPI_SendCommand(ST7789_SWRESET);
  // osDelay(150);
#endif
  osDelay(1000);

  /* 显存访问控制：竖屏，从上到下、从左到右，RGB 格式（与鹿小班 0x36,0x00 一致） */
  SPI_SendCommand(ST7789_MADCTL);
  SPI_SendData(ST7789_MADCTL_VERTICAL);

  /* 接口像素格式：16 位 RGB565 */
  SPI_SendCommand(ST7789_COLMOD);
  SPI_SendData(ST7789_COLOR_MODE_16bit);

  /* Porch 控制 */
  SPI_SendCommand(0xB2);
  {
    uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
    SPI_SendBuffer(data, sizeof(data));
  }

  /* 内部电压设置（厂家推荐值） */
  SPI_SendCommand(0xB7);   /* 栅极电压设置 */
  SPI_SendData(0x35);
  SPI_SendCommand(0xBB);  /* 公共电压 VCOM */
  SPI_SendData(0x19);
  SPI_SendCommand(0xC0); /* LCM 控制 */
  SPI_SendData(0x2C);
  SPI_SendCommand(0xC2);  /* VDV 和 VRH 来源 */
  SPI_SendData(0x01);
  SPI_SendCommand(0xC3);  /* VRH 电压 */
  SPI_SendData(0x12);
  SPI_SendCommand(0xC4);  /* VDV 电压 */
  SPI_SendData(0x20);
  SPI_SendCommand(0xC6);  /* 正常模式帧率 */
  SPI_SendData(0x0F); /* 60Hz */
  SPI_SendCommand(0xD0);  /* 电源控制 */
  SPI_SendData(0xA4);
  SPI_SendData(0xA1);

  /* 伽马校正 */
  SPI_SendCommand(0xE0);
  {
    uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F,
                      0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
    SPI_SendBuffer(data, sizeof(data));
  }
  SPI_SendCommand(0xE1);
  {
    uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F,
                      0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
    SPI_SendBuffer(data, sizeof(data));
  }

  /* 打开反显（常黑型面板需反显） */
  SPI_SendCommand(ST7789_INVON);

  /* 退出休眠，LCD 上电/复位后默认处于休眠 */
  SPI_SendCommand(ST7789_SLPOUT);
  osDelay(120); /* 数据手册要求：Sleep Out 后需等待 120ms 使电源和时钟稳定 */

  /* 打开显示 */
  SPI_SendCommand(ST7789_DISPON);

  osDelay(50);
  LCD_SetRotation(NO_ROTATION); /* 设置显示方向 */
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
  if ((pPixel->x < 0) || (pPixel->x >= ST7789_WIDTH) || (pPixel->y < 0) ||
      (pPixel->y >= ST7789_HEIGHT))
    return;
#ifdef USE_BUFFER
  tmp_color.u16Data = ColorToRGB565(pPixel->color);
  ConvertHalfWord2BigEndian(&tmp_color, uColorData);
  memcpy(lcd_buffer+(pPixel->x+pPixel->y*ST7789_WIDTH)*2,uColorData,2);
#else
  LCD_SetAddressWindow(pPixel->x, pPixel->y, pPixel->x, pPixel->y);
  tmp_color.u16Data = ColorToRGB565(pPixel->color);
  ConvertHalfWord2BigEndian(&tmp_color, uColorData);
  SPI_SendBuffer(uColorData, sizeof(uColorData));
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
