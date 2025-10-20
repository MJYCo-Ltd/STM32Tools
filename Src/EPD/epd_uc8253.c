#include "EPD/epd_uc8253.h"
#include "EPD/epd_uc8253_User.c"

#define EPD_COLOR_WHITE 0xFF
#define EPD_COLOR_BLACK 0x00
#define EPD_COLOR_RED 0x00
#define EPD_COLOR_ALPHA 0xFF

///设置命令
// UC8253 Command Table
#define EPD_CMD_PANEL_SETTING                     0x00
#define EPD_CMD_POWER_SETTING                     0x01
#define EPD_CMD_POWER_OFF                         0x02
#define EPD_CMD_POWER_ON                          0x04
#define EPD_CMD_BOOSTER_SOFT_START                0x06
#define EPD_CMD_DEEP_SLEEP                        0x07
#define EPD_CMD_DATA_START_TRANSMISSION_1         0x10
#define EPD_CMD_DATA_STOP                         0x11
#define EPD_CMD_DISPLAY_REFRESH                   0x12
#define EPD_CMD_DATA_START_TRANSMISSION_2         0x13
#define EPD_CMD_AUTO_SEQUENCE                     0x17
#define EPD_CMD_VCOM_LUT                          0x20
#define EPD_CMD_W2W_LUT                           0x21
#define EPD_CMD_B2W_LUT                           0x22
#define EPD_CMD_W2B_LUT                           0x23
#define EPD_CMD_B2B_LUT                           0x24
#define EPD_CMD_PLL_CONTROL                       0x30
#define EPD_CMD_TEMPERATURE_SENSOR_CALIBRATION    0x40
#define EPD_CMD_TEMPERATURE_SENSOR_SELECTION      0x41
#define EPD_CMD_TEMPERATURE_SENSOR_WRITE          0x42
#define EPD_CMD_TEMPERATURE_SENSOR_READ           0x43
#define EPD_CMD_VCOM_AND_DATA_INTERVAL_SETTING    0x50
#define EPD_CMD_TCON_SETTING                      0x60
#define EPD_CMD_RESOLUTION_SETTING                0x61
#define EPD_CMD_VCM_DC_SETTING                    0x82
#define EPD_CMD_PARTIAL_WINDOW                    0x90
#define EPD_CMD_PARTIAL_IN                        0x91
#define EPD_CMD_PARTIAL_OUT                       0x92

///!设置命令

// ===================== LUT 全刷新 =====================
static const uint8_t lut_full_update[] = {
    0x80, 0x48, 0x40, 0x00, 0x00, 0x00, 0x40, 0x48, 0x80, 0x00, 0x00,
    0x00, 0x80, 0x48, 0x40, 0x00, 0x00, 0x00, 0x40, 0x48, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F,
    0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00};

// ===================== LUT 局部刷新 =====================
static const uint8_t lut_partial_update[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x13, 0x11, 0x00,
    0x00, 0x00, 0x00, 0x13, 0x11, 0x00, 0x00, 0x00, 0x00};

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

void EPD_Sleep(void) {
  EPD_SendCommand(EPD_CMD_POWER_OFF);
  EPD_SendCommand(EPD_CMD_DEEP_SLEEP);
  EPD_SendData(0xA5);
}

void EPD_LoadLUT(const uint8_t *lut, uint8_t is_partial) {
  if (is_partial) {
    EPD_SendCommand(EPD_CMD_W2W_LUT); // 局刷 LUT 地址
  } else {
    EPD_SendCommand(EPD_CMD_VCOM_LUT); // 全刷 LUT 地址
  }

  for (uint8_t i = 0; i < 42; i++) {
    EPD_SendData(lut[i]);
  }
}

// 全屏刷新
void EPD_DisplayFull(uint8_t *buffer) {
  EPD_LoadLUT(lut_full_update, 0);
  EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
  for (uint32_t i = 0; i < (EPD_WIDTH * EPD_HEIGHT) / 8; i++) {
    EPD_SendData(buffer[i]);
  }
  EPD_SendCommand(EPD_CMD_DISPLAY_REFRESH);
}

// 局部刷新（快速，不闪屏）
void EPD_DisplayPartialBuffer(uint8_t *buffer) {
  EPD_LoadLUT(lut_partial_update, 1);

  EPD_SendCommand(EPD_CMD_PARTIAL_IN); // 进入局部模式
  EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
  for (uint32_t i = 0; i < (EPD_WIDTH * EPD_HEIGHT) / 8; i++) {
    EPD_SendData(buffer[i]);
  }
  EPD_SendCommand(EPD_CMD_DISPLAY_REFRESH);
  EPD_SendCommand(EPD_CMD_PARTIAL_OUT); // 退出局部模式
}

// ================= 初始化 =================
void EPD_Init(void) {
  // 硬复位
  EPD_Rest();
  EPD_SendCommand(EPD_CMD_PANEL_SETTING);
  EPD_SendData(0x0F); // 240×416 + 反向扫描
  EPD_WaitUntilIdle();
}

// ================= 显示相关 =================
void EPD_Clear(EPD_COLOR color) {

  // 黑白屏为老数据
  // 黑白红屏为黑白数据
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
    EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_1);
    for (uint16_t i = 0; i < EPD_BUFFER_SIZE; i++) {
      EPD_SendData(EPD_COLOR_BLACK);
    }
    EPD_WaitUntilIdle();
    EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
    for (uint16_t i = 0; i < EPD_BUFFER_SIZE; i++) {
      EPD_SendData(EPD_COLOR_RED);
    }
    break;
  }
  EPD_WaitUntilIdle();
}

void EPD_Done(void) {
  /// 根据文档 下面的命令就是 PON->DRF->POF->DSLP
  /// 但这个命令只能使用一次，不知道为什么
  // EPD_SendCommand(EPD_CMD_AUTO_SEQUENCE);
  // EPD_SendData(0xA5);
  // EPD_WaitUntilIdle();
  EPD_SendCommand(EPD_CMD_POWER_ON);
  EPD_WaitUntilIdle();

  EPD_SendCommand(EPD_CMD_DISPLAY_REFRESH);
  EPD_WaitUntilIdle();

  EPD_Sleep();

  osDelay(10);
}

uint8_t EPD_GetInnerTemp() { return (0x19); }
// ================= 局部刷新 =================
void EPD_DisplayPartial(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        const uint8_t *data) {
  if ((x + w) > EPD_WIDTH || (y + h) > EPD_HEIGHT)
    return;

  uint16_t x_start = x & 0xFFF8; // 对齐到8位
  uint16_t x_end = x + w - 1;
  uint16_t y_start = y;
  uint16_t y_end = y + h - 1;

  // 进入部分刷新模式
  EPD_SendCommand(EPD_CMD_PARTIAL_IN); // partial in

  // 设置窗口
  EPD_SendCommand(EPD_CMD_PARTIAL_WINDOW);
  EPD_SendData((x_start >> 8) & 0xFF);
  EPD_SendData(x_start & 0xFF);
  EPD_SendData((x_end >> 8) & 0xFF);
  EPD_SendData(x_end & 0xFF);
  EPD_SendData((y_start >> 8) & 0xFF);
  EPD_SendData(y_start & 0xFF);
  EPD_SendData((y_end >> 8) & 0xFF);
  EPD_SendData(y_end & 0xFF);
  EPD_SendData(0x01); // enable

  // 写入图像数据
  EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
  for (uint32_t i = 0; i < (w * h) / 8; i++) {
    EPD_SendData(data[i]);
  }

  // 刷新该区域
  EPD_SendCommand(EPD_CMD_DISPLAY_REFRESH);

  // 退出部分刷新模式
  EPD_SendCommand(EPD_CMD_PARTIAL_OUT);
}
