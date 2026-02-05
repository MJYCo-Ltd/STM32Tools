/*
 * tmp117.c
 *
 *  Created on: Jan 13, 2026
 *      Author: TMP117 Driver
 */
#include <stddef.h>
#include "Base.h"
#include "TMP/tmp117.h"

// ---- Register pointer addresses (spec Table 7-3) ----
#define TMP117_REG_TEMP_RES      0x00
#define TMP117_REG_CONFIGURATION 0x01
#define TMP117_REG_T_HIGH        0x02
#define TMP117_REG_T_LOW         0x03
#define TMP117_REG_EE_UNLOCK     0x04
#define TMP117_REG_EEPROM1       0x05
#define TMP117_REG_EEPROM2       0x06
#define TMP117_REG_TEMP_OFFSET   0x07
#define TMP117_REG_EEPROM3       0x08
#define TMP117_REG_DEVICE_ID     0x0F

// ---- Configuration bits (spec Figure 7-14 & Table 7-6) ----
#define TMP117_CFG_HIGH_ALERT_Pos   15
#define TMP117_CFG_LOW_ALERT_Pos    14
#define TMP117_CFG_DATA_READY_Pos   13
#define TMP117_CFG_EE_BUSY_Pos      12
#define TMP117_CFG_MOD1_Pos         11
#define TMP117_CFG_MOD0_Pos         10
#define TMP117_CFG_CONV2_Pos         9
#define TMP117_CFG_CONV1_Pos         8
#define TMP117_CFG_CONV0_Pos         7
#define TMP117_CFG_AVG1_Pos          6
#define TMP117_CFG_AVG0_Pos          5
#define TMP117_CFG_TnA_Pos           4  // 1: Therm, 0: Alert
#define TMP117_CFG_POL_Pos           3  // 1: Active high
#define TMP117_CFG_DR_ALERT_Pos      2  // 1: ALERT = DataReady
#define TMP117_CFG_SOFT_RESET_Pos    1  // write 1 triggers ~2ms reset

#define TMP117_DEVICE_ID_VALUE       0x0117

// Conversion cycle presets (spec Table 7-7)
typedef enum {
    TMP117_CONV_15_5_MS  = 0b000,
    TMP117_CONV_125_MS   = 0b001,
    TMP117_CONV_250_MS   = 0b010,
    TMP117_CONV_500_MS   = 0b011,
    TMP117_CONV_1_S      = 0b100,
    TMP117_CONV_4_S      = 0b101,
    TMP117_CONV_8_S      = 0b110,
    TMP117_CONV_16_S     = 0b111,
} tmp117_conv_t;

// Averaging (spec Table 7-6 AVG[1:0])
typedef enum {
    TMP117_AVG_NONE  = 0b00,
    TMP117_AVG_8     = 0b01,
    TMP117_AVG_32    = 0b10,
    TMP117_AVG_64    = 0b11,
} tmp117_avg_t;

#ifdef PLATFORM_STM32
#include "TMP/tmp117_User.c"
#else
TMP117_Status TMP117_I2C_Write(uint8_t addr7, uint8_t reg, uint16_t val, int type);
TMP117_Status TMP117_I2C_Read(uint8_t addr7, uint8_t reg, uint16_t *val, int type);
#endif

// 获取温度值
TMP117_Status TMP117_GetTemperature(uint8_t addr7, TMP117_Temp* temp) {
    if (temp == NULL) {
        return TMP117_ERR_RANGE;
    }
    
    TMP117_Status status = TMP117_I2C_Read(addr7, TMP117_REG_TEMP_RES, &temp->uValue,2);
    if (status != TMP117_OK) {
        return status;
    }
    
    // 转换为摄氏度 (1 LSB = 0.0078125°C)
    temp->value = ((int16_t)temp->uValue) * 0.0078125f;
    
    // 检查是否在医学有效区间（通常为 35-42°C）
    temp->valid = (temp->value >= 35.0f && temp->value <= 42.0f);
    
    return TMP117_OK;
}

// 设置工作模式
TMP117_Status TMP117_SetWorkMode(uint8_t addr7, TMP117_Mode workMode) {
    // 读取当前配置寄存器
    uint16_t config_raw;
    TMP117_Status status = TMP117_I2C_Read(addr7, TMP117_REG_CONFIGURATION, &config_raw,2);
    if (status != TMP117_OK) {
        return status;
    }

    // 清除模式位（MOD[1:0] 在 bit 10-11）
    config_raw &= ~(TMP117_MODE_ONE_SHOT << TMP117_CFG_MOD0_Pos);

    // 设置新的模式位
    config_raw |= ((uint16_t)workMode << TMP117_CFG_MOD0_Pos);

    // 写回配置寄存器
    status = TMP117_I2C_Write(addr7, TMP117_REG_CONFIGURATION, config_raw,2);
    if (status != TMP117_OK) {
        return status;
    }

    return TMP117_OK;
}
