/*
 ******************************************************************************
 * @file           : TMP117.h
 * @brief          : Header for TMP117.c file.
 *                   此文件为TMP117传感器的驱动
 ******************************************************************************
 *
 *  Created on: Dec 24, 2025
 *      Author: yty
 */
// tmp117.h
#ifndef YTY_TMP117_H
#define YTY_TMP117_H

#include "main.h"  // replace with your MCU HAL header
#include <stdint.h>
#include <stdbool.h>

// ---- TMP117 I2C 7-bit addresses by ADD0 pin (spec Table 7-2) ----
#define TMP117_ADDR_GND    (0x48)  // 1001000
#define TMP117_ADDR_VDD    (0x49)  // 1001001
#define TMP117_ADDR_SDA    (0x4A)  // 1001010
#define TMP117_ADDR_SCL    (0x4B)  // 1001011

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

// Modes (spec Table 7-6 MOD[1:0])
typedef enum {
    TMP117_MODE_CONTINUOUS = 0b00, // 10 reads back as 00
    TMP117_MODE_SHUTDOWN   = 0b01,
    TMP117_MODE_ONE_SHOT   = 0b11,
} tmp117_mode_t;

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t i2c_addr7; // 7-bit address: 0x48..0x4B
} tmp117_t;

// ---- API ----
HAL_StatusTypeDef tmp117_read_u16(tmp117_t *dev, uint8_t reg, uint16_t *val);
HAL_StatusTypeDef tmp117_write_u16(tmp117_t *dev, uint8_t reg, uint16_t val);

HAL_StatusTypeDef tmp117_read_temperature_raw(tmp117_t *dev, int16_t *raw);
HAL_StatusTypeDef tmp117_read_temperature_c(tmp117_t *dev, float *degC);

HAL_StatusTypeDef tmp117_read_config(tmp117_t *dev, uint16_t *cfg);
HAL_StatusTypeDef tmp117_write_config(tmp117_t *dev, uint16_t cfg);

HAL_StatusTypeDef tmp117_set_mode(tmp117_t *dev, tmp117_mode_t mode);
HAL_StatusTypeDef tmp117_set_conv_avg(tmp117_t *dev, tmp117_conv_t conv, tmp117_avg_t avg);
HAL_StatusTypeDef tmp117_select_alert_mode(tmp117_t *dev, bool therm_mode, bool alert_pin_active_high, bool alert_pin_dataready);

HAL_StatusTypeDef tmp117_set_limits(tmp117_t *dev, int16_t high_raw, int16_t low_raw);
HAL_StatusTypeDef tmp117_set_offset(tmp117_t *dev, int16_t offset_raw);

HAL_StatusTypeDef tmp117_read_device_id(tmp117_t *dev, uint16_t *did);
HAL_StatusTypeDef tmp117_read_status_flags(tmp117_t *dev, bool *high_alert, bool *low_alert, bool *data_ready, bool *ee_busy);

HAL_StatusTypeDef tmp117_eeprom_unlock(tmp117_t *dev, bool unlock);
HAL_StatusTypeDef tmp117_eeprom_busy(tmp117_t *dev, bool *busy);
HAL_StatusTypeDef tmp117_write_eeprom_gp(tmp117_t *dev, uint8_t ee_reg_05_06_08, uint16_t data);

HAL_StatusTypeDef tmp117_soft_reset(tmp117_t *dev);
HAL_StatusTypeDef tmp117_general_call_reset(I2C_HandleTypeDef *hi2c);

void TMP117_Init_Axilla(tmp117_t*);
void TMP117_Read_Axilla(tmp117_t*,float*);

#endif // YTY_TMP117_H
