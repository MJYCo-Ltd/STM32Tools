#include "EPD/epd_uc8253.h"
#include <string.h>

uint8_t g_model = EPD_THREE_COLOR;
#define EPD_COLOR_WHITE 0xFF
#define EPD_COLOR_BLACK 0x00
#define EPD_COLOR_RED 0x00
#define EPD_COLOR_ALPHA 0xFF

/// 设置命令
/// 参考 UC8253.pdf
#define EPD_CMD_PANEL_SETTING 0x00
#define EPD_CMD_POWER_SETTING 0x01
#define EPD_CMD_POWER_OFF 0x02
#define EPD_CMD_POWER_ON 0x04
#define EPD_CMD_BOOSTER_SOFT_START 0x06
#define EPD_CMD_DEEP_SLEEP 0x07
#define EPD_CMD_DATA_START_TRANSMISSION_1 0x10
#define EPD_CMD_DATA_STOP 0x11
#define EPD_CMD_DISPLAY_REFRESH 0x12
#define EPD_CMD_DATA_START_TRANSMISSION_2 0x13
#define EPD_CMD_AUTO_SEQUENCE 0x17
#define EPD_CMD_VCOM_LUT 0x20
#define EPD_CMD_W2W_LUT 0x21
#define EPD_CMD_B2W_LUT 0x22
#define EPD_CMD_W2B_LUT 0x23
#define EPD_CMD_B2B_LUT 0x24
#define EPD_CMD_PLL_CONTROL 0x30
#define EPD_CMD_TEMPERATURE_SENSOR_CALIBRATION 0x40
#define EPD_CMD_TEMPERATURE_SENSOR_SELECTION 0x41
#define EPD_CMD_TEMPERATURE_SENSOR_WRITE 0x42
#define EPD_CMD_TEMPERATURE_SENSOR_READ 0x43
#define EPD_CMD_PANEL_GLASS_CHECK 0x44
#define EPD_CMD_VCOM_AND_DATA_INTERVAL_SETTING 0x50
#define EPD_CMD_TCON_SETTING 0x60
#define EPD_CMD_RESOLUTION_SETTING 0x61
#define EPD_CMD_VCM_DC_SETTING 0x82
#define EPD_CMD_PARTIAL_WINDOW 0x90
#define EPD_CMD_PARTIAL_IN 0x91
#define EPD_CMD_PARTIAL_OUT 0x92
#define EPD_CMD_CASCADE_SETTING 0xE0
#define EPD_CMD_FORCE_TEMPERATURE 0xE5
///!设置命令

/// EPD_CMD_PANEL_SETTING 对应的参数
#define PANEL_SETTING_REG (1 << 5)
#define PANEL_SETTING_KW (1 << 4) // 黑白两色
///!EPD_CMD_PANEL_SETTING

/// ---------- 快速刷新 LUT（参考 UC8253.pdf） ----------
/// 说明：此 LUT 经过简化，用于快刷（降低闪烁 / 提速）
/// 每个 LUT 对应不同的灰阶转换（W2W, K2W, W2K, K2K）
#ifndef GOOD_DISPLAY
// ---- Minimal working fast LUT (可显、对比度低但有反应) ----
static const uint8_t FAST_LUTC[57] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Frame time
    0x02, 0x08, 0x08, 0x02,                         // Level select (正负翻转)
    0x10, 0x18, 0x10, 0x18,                         // 驱动电压切换
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 填充
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t FAST_LUTBW[57] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08, 0x08, 0x02,
    0x10, 0x18, 0x10, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif
// 底层 SPI 通信
void EPD_SendCommand(uint8_t cmd);
void EPD_SendData(uint8_t data);
void EPD_SendBuffer(const unsigned char *pBuffer, uint16_t unLength);

#include "EPD/epd_uc8253_User.c"

void EPD_LoadFastLUT(void) {
#ifdef GOOD_DISPLAY
  EPD_SendCommand(EPD_CMD_CASCADE_SETTING);
  EPD_SendData(0x02);

  EPD_SendCommand(EPD_CMD_FORCE_TEMPERATURE);
  EPD_SendData(0x5F); // 0x5F--1.5s
#else
  EPD_SendCommand(EPD_CMD_VCOM_LUT);
  EPD_SendBuffer(FAST_LUTC, sizeof(FAST_LUTC));
  EPD_WaitUntilIdle();
  EPD_SendCommand(EPD_CMD_W2W_LUT);
  EPD_SendBuffer(FAST_LUTBW, sizeof(FAST_LUTBW));
  EPD_WaitUntilIdle();
  EPD_SendCommand(EPD_CMD_B2W_LUT);
  EPD_SendBuffer(FAST_LUTBW, sizeof(FAST_LUTBW));
  EPD_WaitUntilIdle();
  EPD_SendCommand(EPD_CMD_W2B_LUT);
  EPD_SendBuffer(FAST_LUTBW, sizeof(FAST_LUTBW));
  EPD_WaitUntilIdle();
  EPD_SendCommand(EPD_CMD_B2B_LUT);
  EPD_SendBuffer(FAST_LUTBW, sizeof(FAST_LUTBW));
  EPD_WaitUntilIdle();
#endif
}
/**
 * @brief epd_WBframe 黑白像素
 * @brief epd_RFrame  红色像素
 */
uint8_t epd_WBframe[EPD_BUFFER_SIZE], epd_Rframe[EPD_BUFFER_SIZE];
// ================= 低层通信函数 =================
void EPD_SendCommand(uint8_t cmd) {
  SELECT_EPD;
  EPD_CMD;
  SPI_Write(cmd);
  UNSELECT_EPD;
}

void EPD_SendData(uint8_t data) {
  SELECT_EPD;
  EPD_DATA;
  SPI_Write(data);
  UNSELECT_EPD;
}

void EPD_SendBuffer(const unsigned char *pBuffer, uint16_t unLength) {
  SELECT_EPD;
  EPD_DATA;
  SPI_WriteBuffer(pBuffer, unLength);
  UNSELECT_EPD;
}

/// 开机
void EPD_PowerOn(void) {
  EPD_SendCommand(EPD_CMD_POWER_ON);
  EPD_WaitUntilIdle();
}

/// 关机
void EPD_PowerOff(void) {
  EPD_SendCommand(EPD_CMD_POWER_OFF);
  EPD_WaitUntilIdle();
}

/// 硬件休眠
void EPD_DeepSleep(void) {
  EPD_SendCommand(EPD_CMD_DEEP_SLEEP);
  EPD_SendData(0xA5);
}

/// 初始化显存
void EPD_InitDrawBuffer(EPD_COLOR color) {
  if (EPD_TWO_COLOR == g_model && EPD_RED == color) {
    return;
  }
  if (EPD_RED == color) {
    memset(epd_WBframe, EPD_COLOR_WHITE, EPD_BUFFER_SIZE);
    memset(epd_Rframe, EPD_COLOR_RED, EPD_BUFFER_SIZE);
  } else {
    if (EPD_THREE_COLOR == g_model) {
      memset(epd_Rframe, EPD_COLOR_ALPHA, EPD_BUFFER_SIZE);
    }
    if (EPD_BLACK == color) {
      memset(epd_WBframe, EPD_COLOR_BLACK, EPD_BUFFER_SIZE);
    } else {
      memset(epd_WBframe, EPD_COLOR_WHITE, EPD_BUFFER_SIZE);
    }
  }
}

void EPD_ShowBuffer(void) {
  EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_1);
  EPD_SendBuffer(epd_WBframe, EPD_BUFFER_SIZE);
  EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
  EPD_SendBuffer(EPD_THREE_COLOR == g_model ? epd_Rframe : epd_WBframe,
                 EPD_BUFFER_SIZE);
}

/// 给每个像素上颜色
void EPD_DrawPixel(const EPD_Pixel *pPixel) {
  if (pPixel->x >= EPD_WIDTH || pPixel->y >= EPD_HEIGHT) {
    return;
  }
  if (EPD_TWO_COLOR == g_model && EPD_RED == pPixel->color) {
    return;
  }

  uint32_t index = pPixel->x + pPixel->y * EPD_WIDTH;
  uint32_t byte_index = index / 8;
  uint8_t bit_mask = 0x80 >> (index % 8);
  if (pPixel->color == EPD_RED) {
    epd_Rframe[byte_index] &= ~bit_mask;
  } else {
    if (EPD_THREE_COLOR == g_model) {
      epd_Rframe[byte_index] |= bit_mask;
    }
    if (pPixel->color == EPD_BLACK) {
      epd_WBframe[byte_index] &= ~bit_mask;
    } else {
      epd_WBframe[byte_index] |= bit_mask;
    }
  }
}

// ================= 初始化 =================
void EPD_Init(EPD_MODEL model, uint8_t fastFresh) {
  // 硬复位
  g_model = model;
  EPD_Rest();
  EPD_SendCommand(EPD_CMD_PANEL_SETTING);
  uint8_t panelSetting = 0x0F;
  if (EPD_TWO_COLOR == g_model) {
    panelSetting |= PANEL_SETTING_KW;
  }
#ifndef GOOD_DISPLAY
  if (0 != fastFresh) {
    panelSetting |= PANEL_SETTING_REG;
  }
#endif
  EPD_SendData(panelSetting);
  EPD_WaitUntilIdle();

  if (0 != fastFresh) {
    EPD_SendCommand(EPD_CMD_POWER_ON);
    EPD_WaitUntilIdle();
    EPD_LoadFastLUT();
  }
#ifdef GOOD_DISPLAY
  else {
    EPD_SendCommand(EPD_CMD_POWER_ON);
    EPD_WaitUntilIdle();
    EPD_SendCommand(EPD_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
    EPD_SendData(0x97);
  }
#endif
}

// ================= 显示相关 =================
void EPD_Clear(EPD_COLOR color) {
  // 黑白屏为老数据
  // 黑白红屏为黑白数据
  if (EPD_TWO_COLOR == g_model && EPD_RED == color) {
    return;
  }
  if (EPD_THREE_COLOR == g_model) {
    switch (color) {
    case EPD_WHITE:
    case EPD_BLACK:
      EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_1);
      uint8_t colorBuffer =
          EPD_WHITE == color ? EPD_COLOR_WHITE : EPD_COLOR_BLACK;
      for (uint16_t i = 0; i < EPD_BUFFER_SIZE; i++) {
        EPD_SendData(colorBuffer);
      }
      EPD_WaitUntilIdle();
      EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
      for (uint16_t i = 0; i < EPD_BUFFER_SIZE; i++) {
        EPD_SendData(EPD_COLOR_ALPHA);
      }
      break;
    case EPD_RED:
      // 在黑白屏此处为新数据，黑白红三色屏此处为红色的数据
      EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
      for (uint16_t i = 0; i < EPD_BUFFER_SIZE; i++) {
        EPD_SendData(EPD_COLOR_RED);
      }
      break;
    }
  } else {
    EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_1);
    uint8_t colorBuffer =
        EPD_WHITE == color ? EPD_COLOR_WHITE : EPD_COLOR_BLACK;
    for (uint16_t i = 0; i < EPD_BUFFER_SIZE; i++) {
      EPD_SendData(epd_WBframe[i]);
    }
    EPD_WaitUntilIdle();
    EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
    for (uint16_t i = 0; i < EPD_BUFFER_SIZE; i++) {
      EPD_SendData(colorBuffer);
      epd_WBframe[i] = colorBuffer;
    }
  }
  EPD_WaitUntilIdle();
}

void EPD_Update(void) {
  EPD_SendCommand(EPD_CMD_DISPLAY_REFRESH);
  EPD_WaitUntilIdle();
}

// ================= 局部刷新 =================
void EPD_DisplayPartial(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        const EPD_Pixel *data) {
  if ((x + w) > EPD_WIDTH || (y + h) > EPD_HEIGHT)
    return;

  uint16_t x_start = x & 0xFFF8; // 对齐到8位
  uint16_t end = x + w - 1;
  uint16_t x_end = end % 8 == 0 ? (end + 7) : end;
  uint16_t y_end = y + h - 1;

  // 进入部分刷新模式
  EPD_SendCommand(EPD_CMD_PARTIAL_IN);

  // 设置窗口
  EPD_SendCommand(EPD_CMD_PARTIAL_WINDOW);
  EPD_SendData(x_start);
  EPD_SendData(x_end);
  EPD_SendData((y >> 8) & 0xFF);
  EPD_SendData(y & 0xFF);
  EPD_SendData((y_end >> 8) & 0xFF);
  EPD_SendData(y_end & 0xFF);
  EPD_SendData(0x00);

  // 写入图像数据
  // EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_1);
  // for (uint16_t i = 0; i < (w * h) / 8; i++) {
  //   EPD_SendData(EPD_COLOR_BLACK);
  // }
  // EPD_WaitUntilIdle();
  EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
  for (uint16_t i = 0; i < ((x_end - x_start) * h) / 8; i++) {
    EPD_SendData(EPD_COLOR_BLACK);
  }
  EPD_WaitUntilIdle();
  EPD_Update();
  /// 退出局部模式
  EPD_SendCommand(EPD_CMD_PARTIAL_OUT);
}
