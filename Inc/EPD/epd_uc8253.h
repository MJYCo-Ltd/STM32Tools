#ifndef __EPD_UC8253_H__
#define __EPD_UC8253_H__

#include <stdint.h>

#define EPD_COLOR_WHITE 0xFF
#define EPD_COLOR_BLACK 0x00

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

///跟硬件密切相关的，需要根据硬件修改
void EPD_Rest(void);
void EPD_WaitUntilIdle();
///!跟硬件密切相关的，需要根据硬件修改

// 初始化与基本操作
void EPD_Init(void);
void EPD_Wakeup(void);
void EPD_Clear(void);
void EPD_DisplayFrame(void);
void EPD_Sleep(void);

// 局部刷新接口
void EPD_DisplayPartial(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);

// 底层 SPI 通信
void EPD_SendCommand(uint8_t cmd);
void EPD_SendData(uint8_t data);
void EPD_SendBuffer(const unsigned char* pBuffer,uint16_t unLength);
void EPD_Display(const uint8_t *image);
void EPD_Display_Clear(void);

#endif
