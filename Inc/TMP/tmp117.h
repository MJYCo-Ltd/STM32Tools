/*
 ******************************************************************************
 * @file           : tmp117.h
 * @brief          : Reusable TMP117 I2C temperature sensor driver
 ******************************************************************************
 */
#ifndef STM32TOOLS_TMP117_H
#define STM32TOOLS_TMP117_H

#include <stdint.h>

#include "Bus/i2c_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TMP117_ADDR_GND 0x48U
#define TMP117_ADDR_VDD 0x49U
#define TMP117_ADDR_SDA 0x4AU
#define TMP117_ADDR_SCL 0x4BU

typedef enum {
  TMP117_OK = 0,
  TMP117_ERR_PARAM,
  TMP117_ERR_WRITE_I2C,
  TMP117_ERR_READ_I2C,
  TMP117_ERR_TIMEOUT,
  TMP117_ERR_RANGE,
  TMP117_ERR_NOT_READY
} TMP117_Status;

typedef enum {
  TMP117_MODE_CONTINUOUS = 0U,
  TMP117_MODE_SHUTDOWN = 1U,
  TMP117_MODE_ONE_SHOT = 3U
} TMP117_Mode;

typedef enum {
  TMP117_AVERAGE_NONE = 0U,
  TMP117_AVERAGE_8 = 1U,
  TMP117_AVERAGE_32 = 2U,
  TMP117_AVERAGE_64 = 3U
} TMP117_Averaging;

typedef struct {
  const I2C_Bus *bus;
  uint8_t address7;
} TMP117_Device;

typedef struct {
  uint16_t raw;
  float temperature_c;
} TMP117_Temperature;

/** Configure one TMP117 instance; the borrowed bus must outlive the device. */
TMP117_Status TMP117_DeviceInit(TMP117_Device *device, const I2C_Bus *bus,
                                uint8_t address7);

TMP117_Status TMP117_GetTemperature(const TMP117_Device *device,
                                    TMP117_Temperature *temperature);

/** Read only when the data-ready flag is set. */
TMP117_Status TMP117_GetReadyTemperature(const TMP117_Device *device,
                                         TMP117_Temperature *temperature);

TMP117_Status TMP117_SetWorkMode(const TMP117_Device *device,
                                 TMP117_Mode mode);

/**
 * Set averaging and start one conversion in one configuration write.
 * The sensor returns to shutdown after completion; this does not write EEPROM.
 */
TMP117_Status TMP117_StartOneShot(const TMP117_Device *device,
                                  TMP117_Averaging averaging);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_TMP117_H */
