#include <cmsis_os.h>
#include <Common.h>
#include <Display/LCD/lcd.h>
#include <Display/LCD/lcd_user.c>

HALF_WORD_DATA tmp_color;
uint8_t uColorData[2];

/**
 * @brief 向 ST7789 控制器写入指令
 * @param cmd 要写入的指令
 */
static void ST7789_WriteCommand(uint8_t cmd) {
  ST7789_Select();
  LCD_SEND_CMD;
  HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
  ST7789_UnSelect();
}

/**
 * @brief 向 ST7789 控制器写入数据
 * @param buff 数据缓冲区指针
 * @param buff_size 数据长度
 */
static void ST7789_WriteData(uint8_t *buff, size_t buff_size) {
  ST7789_Select();
  LCD_SEND_DATA;

  /* HAL 单次传输限制 64K，需分块发送 */

  if (buff == lcd_buffer) {
    //SCB_CleanDCache_by_Addr((uint32_t *)lcd_buffer, LCD_BUFFER_SIZE);
  }
  while (buff_size > 0) {
    uint16_t chunk_size = buff_size > 65535 ? 65535 : buff_size;
#ifdef USE_BUFFER
    /*if (16 <= buff_size) {
      HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, buff, chunk_size);
    } else */{
      HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
    }
#else
    HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
#endif
    buff += chunk_size;
    buff_size -= chunk_size;
  }

  ST7789_UnSelect();
}
/**
 * @brief 向 ST7789 写入 8 位数据（简化接口）
 * @param data 要写入的 8 位数据
 */
static void ST7789_WriteSmallData(uint8_t data) {
  ST7789_Select();
  LCD_SEND_DATA;
  HAL_SPI_Transmit(&ST7789_SPI_PORT, &data, sizeof(data), HAL_MAX_DELAY);
  ST7789_UnSelect();
}

/**
 * @brief 设置显示旋转方向
 * @param m 旋转参数 (NO_ROTATION/ROTATION_90/180/270)
 */
/**
 * @brief 设置显示旋转方向（与鹿小班 LCD_SetDirection 对应）
 *        NO_ROTATION=竖屏, ROTATION_90=横屏, ROTATION_180=竖屏翻转, ROTATION_270=横屏翻转
 */
void LCD_SetRotation(ROTATION m) {
  ST7789_WriteCommand(ST7789_MADCTL);
  switch (m) {
  case NO_ROTATION:
    ST7789_WriteSmallData(ST7789_MADCTL_VERTICAL); /* 0x00 竖屏 */
    break;
  case ROTATION_90:
    ST7789_WriteSmallData(ST7789_MADCTL_HORIZONTAL); /* 0x70 横屏 */
    break;
  case ROTATION_180:
    ST7789_WriteSmallData(ST7789_MADCTL_V_FLIP); /* 0xC0 竖屏翻转 */
    break;
  case ROTATION_270:
    ST7789_WriteSmallData(ST7789_MADCTL_H_FLIP); /* 0xA0 横屏翻转 */
    break;
  default:
    break;
  }
}

/**
 * @brief 设置显存写入窗口区域
 * @param x0,y0 窗口左上角坐标
 * @param x1,y1 窗口右下角坐标
 */
void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  ST7789_Select();
  uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
  uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;

  /* Column Address set */
  ST7789_WriteCommand(ST7789_CASET);
  {
    uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
    ST7789_WriteData(data, sizeof(data));
  }

  /* Row Address set */
  ST7789_WriteCommand(ST7789_RASET);
  {
    uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
    ST7789_WriteData(data, sizeof(data));
  }
  /* Write to RAM */
  ST7789_WriteCommand(ST7789_RAMWR);
  ST7789_UnSelect();
}

/**
 * @brief 初始化 ST7789 控制器
 * @note  参考鹿小班 lcd_spi_154.c 初始化流程，屏幕配置为 16 位 RGB565 格式
 *        需在 FreeRTOS 启动后调用（使用 osDelay）
 */
void LCD_Init(void) {
#ifdef USE_BUFFER
  memset(lcd_buffer,LCD_BLACK, sizeof(lcd_buffer));
#endif

  /* 复位：硬件复位或软件复位，完成后需等待至少 5ms 才能发送指令 */
#ifndef CFG_NO_REST
  osDelay(10);
  ST7789_RST_Clr();
  osDelay(10);
  ST7789_RST_Set();
  osDelay(20);
#else
   ST7789_WriteCommand(ST7789_SWRESET);
  // osDelay(150);
#endif
  osDelay(1000);

  /* 显存访问控制：竖屏，从上到下、从左到右，RGB 格式（与鹿小班 0x36,0x00 一致） */
  ST7789_WriteCommand(ST7789_MADCTL);
  ST7789_WriteSmallData(ST7789_MADCTL_VERTICAL);

  /* 接口像素格式：16 位 RGB565 */
  ST7789_WriteCommand(ST7789_COLMOD);
  ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);

  /* Porch 控制 */
  ST7789_WriteCommand(0xB2);
  {
    uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
    ST7789_WriteData(data, sizeof(data));
  }

  /* 内部电压设置（厂家推荐值） */
  ST7789_WriteCommand(0xB7);   /* 栅极电压设置 */
  ST7789_WriteSmallData(0x35);
  ST7789_WriteCommand(0xBB);  /* 公共电压 VCOM */
  ST7789_WriteSmallData(0x19);
  ST7789_WriteCommand(0xC0); /* LCM 控制 */
  ST7789_WriteSmallData(0x2C);
  ST7789_WriteCommand(0xC2);  /* VDV 和 VRH 来源 */
  ST7789_WriteSmallData(0x01);
  ST7789_WriteCommand(0xC3);  /* VRH 电压 */
  ST7789_WriteSmallData(0x12);
  ST7789_WriteCommand(0xC4);  /* VDV 电压 */
  ST7789_WriteSmallData(0x20);
  ST7789_WriteCommand(0xC6);  /* 正常模式帧率 */
  ST7789_WriteSmallData(0x0F); /* 60Hz */
  ST7789_WriteCommand(0xD0);  /* 电源控制 */
  ST7789_WriteSmallData(0xA4);
  ST7789_WriteSmallData(0xA1);

  /* 伽马校正 */
  ST7789_WriteCommand(0xE0);
  {
    uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F,
                      0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
    ST7789_WriteData(data, sizeof(data));
  }
  ST7789_WriteCommand(0xE1);
  {
    uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F,
                      0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
    ST7789_WriteData(data, sizeof(data));
  }

  /* 打开反显（常黑型面板需反显） */
  ST7789_WriteCommand(ST7789_INVON);

  /* 退出休眠，LCD 上电/复位后默认处于休眠 */
  ST7789_WriteCommand(ST7789_SLPOUT);
  osDelay(120); /* 数据手册要求：Sleep Out 后需等待 120ms 使电源和时钟稳定 */

  /* 打开显示 */
  ST7789_WriteCommand(ST7789_DISPON);

  osDelay(50);
  LCD_SetRotation(NO_ROTATION); /* 设置显示方向 */
  LCD_Clear(LCD_BLACK);
  LCD_Backlight_ON;
}

/**
 * @brief 全屏填充单色
 * @param color 填充颜色（RGB565）
 */
void LCD_Clear(uint16_t color) {
  LCD_SetAddressWindow(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
  ST7789_Select();

#ifdef USE_BUFFER
    ST7789_WriteData(lcd_buffer, sizeof(lcd_buffer));
#else
  uint16_t j;
  for (i = 0; i < ST7789_WIDTH; i++)
    for (j = 0; j < ST7789_HEIGHT; j++) {
      uint8_t data[] = {color >> 8, color & 0xFF};
      ST7789_WriteData(data, sizeof(data));
    }
#endif
  ST7789_UnSelect();
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
  ST7789_Select();
  ConvertHalfWord2BigEndian(&tmp_color, uColorData);
  ST7789_WriteData(uColorData, sizeof(uColorData));
  ST7789_UnSelect();
#endif
}

/**
 * @brief 填充指定区域为单色
 * @param xSta,ySta 区域左上角坐标
 * @param xEnd,yEnd 区域右下角坐标
 * @param color 填充颜色（RGB565）
 */
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd,
                 uint16_t color) {
  if ((xEnd < 0) || (xEnd >= ST7789_WIDTH) || (yEnd < 0) ||
      (yEnd >= ST7789_HEIGHT))
    return;
  ST7789_Select();
  uint16_t i, j;
  LCD_SetAddressWindow(xSta, ySta, xEnd, yEnd);
  for (i = ySta; i <= yEnd; i++)
    for (j = xSta; j <= xEnd; j++) {
      uint8_t data[] = {color >> 8, color & 0xFF};
      ST7789_WriteData(data, sizeof(data));
    }
  ST7789_UnSelect();
}

/**
 * @brief 全屏颜色反显开关
 * @param invert 1=开启反显，0=关闭
 */
void ST7789_InvertColors(uint8_t invert) {
  ST7789_Select();
  ST7789_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
  ST7789_UnSelect();
}

/**
 * @brief 撕裂效应线开关（用于同步刷新）
 * @param tear 1=开启，0=关闭
 */
void LED_TearEffect(uint8_t tear) {
  ST7789_Select();
  ST7789_WriteCommand(tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */);
  ST7789_UnSelect();
}
