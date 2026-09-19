/*
 ******************************************************************************
 * @file           : aht20.h
 * @brief          : Reusable AHT20 I2C temperature/humidity driver
 ******************************************************************************
 */
#ifndef STM32TOOLS_AHT20_H
#define STM32TOOLS_AHT20_H

#include <stdbool.h>
#include <stdint.h>

#include "Bus/i2c_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/** AHT20 fixed 7-bit I2C address (8-bit write/read addresses 0x70/0x71). */
#define AHT20_I2C_ADDR7 0x38U

typedef enum {
  AHT20_OK = 0,
  AHT20_ERR_PARAM,
  AHT20_ERR_WRITE_I2C,
  AHT20_ERR_READ_I2C,
  AHT20_ERR_BUSY,
  AHT20_ERR_NOT_CALIBRATED,
  AHT20_ERR_CRC,
  AHT20_ERR_TIMEOUT
} AHT20_Status;

typedef void (*AHT20_DelayMsFn)(void *context, uint32_t delay_ms);

typedef struct {
  const I2C_Bus *bus;
  AHT20_DelayMsFn delay_ms;
  void *delay_context;
  uint8_t address7;
} AHT20_Device;

typedef struct {
  float temperature_c;
  float humidity_rh;
  bool valid;
} AHT20_Data;

/**
 * Configure one AHT20 instance. The bus and delay callback are borrowed and
 * must remain valid for every subsequent operation on the device.
 */
AHT20_Status AHT20_DeviceInit(AHT20_Device *device, const I2C_Bus *bus,
                              uint8_t address7, AHT20_DelayMsFn delay_ms,
                              void *delay_context);

/** Wait for power-up and enable calibration when required. */
AHT20_Status AHT20_Initialize(AHT20_Device *device);

/**
 * Trigger and read one blocking measurement (normally at least 80 ms).
 * The datasheet recommends a measurement period greater than one second.
 */
AHT20_Status AHT20_Read(AHT20_Device *device, AHT20_Data *data);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_AHT20_H */
