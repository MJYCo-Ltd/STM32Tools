/*
 ******************************************************************************
 * @file           : aht20.h
 * @brief          : AHT20 温湿度传感器驱动（I2C）
 *                   协议与换算依据 AHT20 说明书（奥松）
 ******************************************************************************
 */
#ifndef __AHT20_H__
#define __AHT20_H__

#include <stdint.h>
#include <stdbool.h>

/** AHT20 固定 7 位 I2C 地址（8 位写地址 0x70 / 读地址 0x71） */
#define AHT20_I2C_ADDR7 (0x38U)

typedef enum {
  AHT20_OK = 0,
  AHT20_ERR_PARAM,
  AHT20_ERR_WRITE_I2C,
  AHT20_ERR_READ_I2C,
  AHT20_ERR_BUSY,
  AHT20_ERR_NOT_CALIBRATED,
  AHT20_ERR_CRC,
  AHT20_ERR_TIMEOUT,
} AHT20_Status;

typedef struct {
  float temperature_c; /**< 温度，单位 °C */
  float humidity_rh;   /**< 相对湿度，单位 %RH */
  bool valid;          /**< 本次读数是否有效 */
} AHT20_Data;

/**
 * @brief 上电初始化：等待上电稳定，必要时发送校准初始化命令
 * @param addr7 7 位 I2C 地址，通常为 AHT20_I2C_ADDR7
 */
AHT20_Status AHT20_Init(uint8_t addr7);

/**
 * @brief 触发一次测量并读取温湿度（阻塞，约 ≥80ms）
 * @note 说明书建议采集周期大于 1 秒/次
 */
AHT20_Status AHT20_Read(uint8_t addr7, AHT20_Data *data);

#endif /* __AHT20_H__ */
