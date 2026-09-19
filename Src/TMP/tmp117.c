#include "TMP/tmp117.h"

#include <stddef.h>
#include <string.h>

#define TMP117_REG_TEMP_RESULT   0x00U
#define TMP117_REG_CONFIGURATION 0x01U
#define TMP117_CFG_MODE_POS      10U
#define TMP117_CFG_AVG_POS       5U
#define TMP117_CFG_DATA_READY    (1U << 13U)

static uint8_t TMP117_DeviceValid(const TMP117_Device *device)
{
  return ((device != NULL) && (device->bus != NULL) &&
          (device->bus->transmit != NULL) && (device->bus->receive != NULL) &&
          (device->address7 <= 0x7FU))
             ? 1U
             : 0U;
}

static TMP117_Status TMP117_MapResult(I2C_BusResult result, uint8_t reading)
{
  if (result == I2C_BUS_OK) {
    return TMP117_OK;
  }
  if (result == I2C_BUS_INVALID_ARGUMENT) {
    return TMP117_ERR_PARAM;
  }
  if (result == I2C_BUS_TIMEOUT) {
    return TMP117_ERR_TIMEOUT;
  }
  return (reading != 0U) ? TMP117_ERR_READ_I2C : TMP117_ERR_WRITE_I2C;
}

static TMP117_Status TMP117_ReadRegister(const TMP117_Device *device,
                                         uint8_t reg, uint16_t *value)
{
  if ((TMP117_DeviceValid(device) == 0U) || (value == NULL)) {
    return TMP117_ERR_PARAM;
  }
  return TMP117_MapResult(
      I2C_BusReadBE16(device->bus, device->address7, reg, value), 1U);
}

static TMP117_Status TMP117_WriteRegister(const TMP117_Device *device,
                                          uint8_t reg, uint16_t value)
{
  if (TMP117_DeviceValid(device) == 0U) {
    return TMP117_ERR_PARAM;
  }
  return TMP117_MapResult(
      I2C_BusWriteBE16(device->bus, device->address7, reg, value), 0U);
}

TMP117_Status TMP117_DeviceInit(TMP117_Device *device, const I2C_Bus *bus,
                                uint8_t address7)
{
  if (device == NULL) {
    return TMP117_ERR_PARAM;
  }

  memset(device, 0, sizeof(*device));
  if ((bus == NULL) || (bus->transmit == NULL) || (bus->receive == NULL) ||
      (address7 > 0x7FU)) {
    return TMP117_ERR_PARAM;
  }

  device->bus = bus;
  device->address7 = address7;
  return TMP117_OK;
}

TMP117_Status TMP117_GetTemperature(const TMP117_Device *device,
                                    TMP117_Temperature *temperature)
{
  TMP117_Status status;

  if (temperature == NULL) {
    return TMP117_ERR_PARAM;
  }

  temperature->raw = 0U;
  temperature->temperature_c = 0.0f;
  status = TMP117_ReadRegister(device, TMP117_REG_TEMP_RESULT,
                               &temperature->raw);
  if (status != TMP117_OK) {
    return status;
  }

  temperature->temperature_c =
      (float)((int16_t)temperature->raw) * 0.0078125f;
  return TMP117_OK;
}

TMP117_Status TMP117_SetWorkMode(const TMP117_Device *device,
                                 TMP117_Mode mode)
{
  uint16_t configuration;
  TMP117_Status status;

  if ((mode != TMP117_MODE_CONTINUOUS) &&
      (mode != TMP117_MODE_SHUTDOWN) &&
      (mode != TMP117_MODE_ONE_SHOT)) {
    return TMP117_ERR_RANGE;
  }

  status =
      TMP117_ReadRegister(device, TMP117_REG_CONFIGURATION, &configuration);
  if (status != TMP117_OK) {
    return status;
  }

  configuration &= (uint16_t)~(3U << TMP117_CFG_MODE_POS);
  configuration |= (uint16_t)mode << TMP117_CFG_MODE_POS;
  return TMP117_WriteRegister(device, TMP117_REG_CONFIGURATION,
                              configuration);
}

TMP117_Status TMP117_StartOneShot(const TMP117_Device *device,
                                  TMP117_Averaging averaging)
{
  uint16_t configuration;
  TMP117_Status status;

  if ((unsigned int)averaging > (unsigned int)TMP117_AVERAGE_64) {
    return TMP117_ERR_RANGE;
  }

  status =
      TMP117_ReadRegister(device, TMP117_REG_CONFIGURATION, &configuration);
  if (status != TMP117_OK) {
    return status;
  }

  configuration &= (uint16_t)~((3U << TMP117_CFG_MODE_POS) |
                               (3U << TMP117_CFG_AVG_POS));
  configuration |=
      ((uint16_t)TMP117_MODE_ONE_SHOT << TMP117_CFG_MODE_POS) |
      ((uint16_t)averaging << TMP117_CFG_AVG_POS);
  return TMP117_WriteRegister(device, TMP117_REG_CONFIGURATION,
                              configuration);
}

TMP117_Status TMP117_GetReadyTemperature(const TMP117_Device *device,
                                         TMP117_Temperature *temperature)
{
  uint16_t configuration;
  TMP117_Status status;

  if (temperature == NULL) {
    return TMP117_ERR_PARAM;
  }

  status =
      TMP117_ReadRegister(device, TMP117_REG_CONFIGURATION, &configuration);
  if (status != TMP117_OK) {
    return status;
  }
  if ((configuration & TMP117_CFG_DATA_READY) == 0U) {
    return TMP117_ERR_NOT_READY;
  }

  return TMP117_GetTemperature(device, temperature);
}
