/*
 * AHT20 reusable driver.
 *
 * Protocol:
 * - trigger: 0xAC 0x33 0x00
 * - result: status + humidity[19:0] + temperature[19:0] + CRC8
 * - CRC8: init 0xFF, polynomial 0x31
 */
#include "AHT20/aht20.h"

#include <stddef.h>
#include <string.h>

#define AHT20_CMD_INIT    0xBEU
#define AHT20_CMD_TRIGGER 0xACU

#define AHT20_STATUS_BUSY_POS 7U
#define AHT20_STATUS_CAL_POS  3U
#define AHT20_STATUS_BUSY_MASK (1U << AHT20_STATUS_BUSY_POS)
#define AHT20_STATUS_CAL_MASK  (1U << AHT20_STATUS_CAL_POS)

#define AHT20_RAW_FULL_SCALE 1048576.0f /* 2^20 */

static uint8_t AHT20_DeviceValid(const AHT20_Device *device)
{
  return ((device != NULL) && (device->bus != NULL) &&
          (device->bus->transmit != NULL) && (device->bus->receive != NULL) &&
          (device->delay_ms != NULL) && (device->address7 <= 0x7FU))
             ? 1U
             : 0U;
}

static AHT20_Status AHT20_TransferResult(I2C_BusResult result,
                                         uint8_t receiving)
{
  if (result == I2C_BUS_OK) {
    return AHT20_OK;
  }
  if (result == I2C_BUS_TIMEOUT) {
    return AHT20_ERR_TIMEOUT;
  }
  if (result == I2C_BUS_INVALID_ARGUMENT) {
    return AHT20_ERR_PARAM;
  }
  return (receiving != 0U) ? AHT20_ERR_READ_I2C : AHT20_ERR_WRITE_I2C;
}

static void AHT20_Delay(const AHT20_Device *device, uint32_t delay_ms)
{
  device->delay_ms(device->delay_context, delay_ms);
}

static AHT20_Status AHT20_Transmit(const AHT20_Device *device,
                                   const uint8_t *data, uint16_t length)
{
  return AHT20_TransferResult(
      I2C_BusTransmit(device->bus, device->address7, data, length), 0U);
}

static AHT20_Status AHT20_Receive(const AHT20_Device *device, uint8_t *data,
                                  uint16_t length)
{
  return AHT20_TransferResult(
      I2C_BusReceive(device->bus, device->address7, data, length), 1U);
}

static uint8_t AHT20_CalcCrc8(const uint8_t *message, uint8_t count)
{
  uint8_t crc = 0xFFU;
  uint8_t byte;
  uint8_t bit;

  for (byte = 0U; byte < count; ++byte) {
    crc ^= message[byte];
    for (bit = 0U; bit < 8U; ++bit) {
      crc = ((crc & 0x80U) != 0U) ? (uint8_t)((crc << 1U) ^ 0x31U)
                                  : (uint8_t)(crc << 1U);
    }
  }
  return crc;
}

static AHT20_Status AHT20_ReadStatus(const AHT20_Device *device,
                                     uint8_t *status)
{
  if (status == NULL) {
    return AHT20_ERR_PARAM;
  }
  return AHT20_Receive(device, status, 1U);
}

static AHT20_Status AHT20_WaitIdle(const AHT20_Device *device,
                                   uint32_t timeout_ms)
{
  uint32_t elapsed = 0U;
  uint8_t status = 0U;
  AHT20_Status result;

  while (elapsed <= timeout_ms) {
    result = AHT20_ReadStatus(device, &status);
    if (result != AHT20_OK) {
      return result;
    }
    if ((status & AHT20_STATUS_BUSY_MASK) == 0U) {
      return AHT20_OK;
    }
    AHT20_Delay(device, 5U);
    elapsed += 5U;
  }
  return AHT20_ERR_TIMEOUT;
}

AHT20_Status AHT20_DeviceInit(AHT20_Device *device, const I2C_Bus *bus,
                              uint8_t address7, AHT20_DelayMsFn delay_ms,
                              void *delay_context)
{
  if (device == NULL) {
    return AHT20_ERR_PARAM;
  }

  memset(device, 0, sizeof(*device));
  if ((bus == NULL) || (bus->transmit == NULL) || (bus->receive == NULL) ||
      (delay_ms == NULL) || (address7 > 0x7FU)) {
    return AHT20_ERR_PARAM;
  }

  device->bus = bus;
  device->address7 = address7;
  device->delay_ms = delay_ms;
  device->delay_context = delay_context;
  return AHT20_OK;
}

AHT20_Status AHT20_Initialize(AHT20_Device *device)
{
  AHT20_Status result;
  uint8_t status = 0U;
  const uint8_t init_cmd[3] = {AHT20_CMD_INIT, 0x08U, 0x00U};

  if (AHT20_DeviceValid(device) == 0U) {
    return AHT20_ERR_PARAM;
  }

  /* Datasheet: wait at least 5 ms after power-up before accessing the bus. */
  AHT20_Delay(device, 40U);

  result = AHT20_ReadStatus(device, &status);
  if (result != AHT20_OK) {
    return result;
  }

  if ((status & AHT20_STATUS_CAL_MASK) == 0U) {
    result = AHT20_Transmit(device, init_cmd, (uint16_t)sizeof(init_cmd));
    if (result != AHT20_OK) {
      return result;
    }
    AHT20_Delay(device, 10U);
    result = AHT20_ReadStatus(device, &status);
    if (result != AHT20_OK) {
      return result;
    }
    if ((status & AHT20_STATUS_CAL_MASK) == 0U) {
      return AHT20_ERR_NOT_CALIBRATED;
    }
  }

  return AHT20_OK;
}

AHT20_Status AHT20_Read(AHT20_Device *device, AHT20_Data *data)
{
  AHT20_Status result;
  uint8_t raw[7];
  uint32_t humidity_raw;
  uint32_t temperature_raw;
  const uint8_t trigger_cmd[3] = {AHT20_CMD_TRIGGER, 0x33U, 0x00U};

  if (data == NULL) {
    return AHT20_ERR_PARAM;
  }

  data->valid = false;
  data->temperature_c = 0.0f;
  data->humidity_rh = 0.0f;

  if (AHT20_DeviceValid(device) == 0U) {
    return AHT20_ERR_PARAM;
  }

  result =
      AHT20_Transmit(device, trigger_cmd, (uint16_t)sizeof(trigger_cmd));
  if (result != AHT20_OK) {
    return result;
  }

  AHT20_Delay(device, 80U);
  result = AHT20_WaitIdle(device, 100U);
  if (result != AHT20_OK) {
    return (result == AHT20_ERR_TIMEOUT) ? AHT20_ERR_BUSY : result;
  }

  result = AHT20_Receive(device, raw, (uint16_t)sizeof(raw));
  if (result != AHT20_OK) {
    return result;
  }

  if ((raw[0] & AHT20_STATUS_BUSY_MASK) != 0U) {
    return AHT20_ERR_BUSY;
  }
  if (AHT20_CalcCrc8(raw, 6U) != raw[6]) {
    return AHT20_ERR_CRC;
  }

  humidity_raw = ((uint32_t)raw[1] << 12U) |
                 ((uint32_t)raw[2] << 4U) |
                 ((uint32_t)raw[3] >> 4U);
  temperature_raw = (((uint32_t)raw[3] & 0x0FU) << 16U) |
                    ((uint32_t)raw[4] << 8U) |
                    (uint32_t)raw[5];

  data->humidity_rh =
      ((float)humidity_raw / AHT20_RAW_FULL_SCALE) * 100.0f;
  data->temperature_c =
      ((float)temperature_raw / AHT20_RAW_FULL_SCALE) * 200.0f - 50.0f;
  data->valid = true;
  return AHT20_OK;
}
