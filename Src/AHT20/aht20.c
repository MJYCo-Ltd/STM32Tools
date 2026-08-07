/*
 * aht20.c
 *
 * AHT20 温湿度驱动，依据说明书：
 * - 写测量命令：0xAC 0x33 0x00，等待约 80ms
 * - 读回 Status + SRH[19:0] + ST[19:0] + CRC
 * - RH = SRH / 2^20 * 100
 * - T  = ST  / 2^20 * 200 - 50
 * - CRC8：初值 0xFF，多项式 X8+X5+X4+1 (0x31)
 */
#include <stddef.h>
#include "Base.h"
#include "AHT20/aht20.h"

#define AHT20_CMD_INIT    0xBEU
#define AHT20_CMD_TRIGGER 0xACU

#define AHT20_STATUS_BUSY_Pos 7
#define AHT20_STATUS_CAL_Pos  3
#define AHT20_STATUS_BUSY_Msk (1U << AHT20_STATUS_BUSY_Pos)
#define AHT20_STATUS_CAL_Msk  (1U << AHT20_STATUS_CAL_Pos)

#define AHT20_RAW_FULL_SCALE 1048576.0f /* 2^20 */

static I2C_Bus s_default_bus;
static const I2C_Bus *s_bus;

#if defined(PLATFORM_STM32) || defined(USE_HAL_DRIVER)
#include "main.h"
extern I2C_HandleTypeDef hi2c1;
#endif

static const I2C_Bus *AHT20_GetBus(void) {
#if defined(PLATFORM_STM32) || defined(USE_HAL_DRIVER)
  if (s_bus == NULL) {
    I2C_BusInitSTM32(&s_default_bus, &hi2c1, 100U);
    s_bus = &s_default_bus;
  }
#endif
  return s_bus;
}

void AHT20_SetBus(const I2C_Bus *bus) { s_bus = bus; }

static AHT20_Status AHT20_TransferResult(I2C_BusResult result,
                                         uint8_t receiving) {
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

static AHT20_Status AHT20_Transmit(uint8_t addr7, const uint8_t *data,
                                   uint16_t length) {
  return AHT20_TransferResult(
      I2C_BusTransmit(AHT20_GetBus(), addr7, data, length), 0U);
}

static AHT20_Status AHT20_Receive(uint8_t addr7, uint8_t *data,
                                  uint16_t length) {
  return AHT20_TransferResult(
      I2C_BusReceive(AHT20_GetBus(), addr7, data, length), 1U);
}

static uint8_t AHT20_CalcCrc8(const uint8_t *message, uint8_t num)
{
  uint8_t crc = 0xFFU;
  uint8_t byte;
  uint8_t i;

  for (byte = 0U; byte < num; ++byte) {
    crc ^= message[byte];
    for (i = 8U; i > 0U; --i) {
      if ((crc & 0x80U) != 0U) {
        crc = (uint8_t)((crc << 1) ^ 0x31U);
      } else {
        crc = (uint8_t)(crc << 1);
      }
    }
  }
  return crc;
}

static AHT20_Status AHT20_ReadStatus(uint8_t addr7, uint8_t *status)
{
  AHT20_Status result;

  if (status == NULL) {
    return AHT20_ERR_PARAM;
  }

  result = AHT20_Receive(addr7, status, 1U);
  return result;
}

static AHT20_Status AHT20_WaitIdle(uint8_t addr7, uint32_t timeout_ms)
{
  uint32_t elapsed = 0U;
  uint8_t status = 0U;
  AHT20_Status result;

  while (elapsed <= timeout_ms) {
    result = AHT20_ReadStatus(addr7, &status);
    if (result != AHT20_OK) {
      return result;
    }
    if ((status & AHT20_STATUS_BUSY_Msk) == 0U) {
      return AHT20_OK;
    }
    YTY_DELAY_MS(5);
    elapsed += 5U;
  }
  return AHT20_ERR_TIMEOUT;
}

AHT20_Status AHT20_Init(uint8_t addr7)
{
  AHT20_Status result;
  uint8_t status = 0U;
  const uint8_t init_cmd[3] = {AHT20_CMD_INIT, 0x08U, 0x00U};

  /* 说明书：上电后需等待 ≥5ms 再操作 SCL/SDA */
  YTY_DELAY_MS(40);

  result = AHT20_ReadStatus(addr7, &status);
  if (result != AHT20_OK) {
    return result;
  }

  /* Bit3=0 表示校准未使能，需发送初始化命令 */
  if ((status & AHT20_STATUS_CAL_Msk) == 0U) {
    result = AHT20_Transmit(addr7, init_cmd, 3U);
    if (result != AHT20_OK) {
      return result;
    }
    YTY_DELAY_MS(10);
    result = AHT20_ReadStatus(addr7, &status);
    if (result != AHT20_OK) {
      return result;
    }
    if ((status & AHT20_STATUS_CAL_Msk) == 0U) {
      return AHT20_ERR_NOT_CALIBRATED;
    }
  }

  return AHT20_OK;
}

AHT20_Status AHT20_Read(uint8_t addr7, AHT20_Data *data)
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

  result = AHT20_Transmit(addr7, trigger_cmd, 3U);
  if (result != AHT20_OK) {
    return result;
  }

  /* 说明书：等待约 80ms 测量完成 */
  YTY_DELAY_MS(80);
  result = AHT20_WaitIdle(addr7, 100U);
  if (result != AHT20_OK) {
    return (result == AHT20_ERR_TIMEOUT) ? AHT20_ERR_BUSY : result;
  }

  result = AHT20_Receive(addr7, raw, 7U);
  if (result != AHT20_OK) {
    return result;
  }

  if ((raw[0] & AHT20_STATUS_BUSY_Msk) != 0U) {
    return AHT20_ERR_BUSY;
  }

  if (AHT20_CalcCrc8(raw, 6U) != raw[6]) {
    return AHT20_ERR_CRC;
  }

  /* SRH[19:0] / ST[19:0] 打包在 byte1..byte5 */
  humidity_raw = ((uint32_t)raw[1] << 12) | ((uint32_t)raw[2] << 4) |
                 ((uint32_t)raw[3] >> 4);
  temperature_raw = (((uint32_t)raw[3] & 0x0FU) << 16) |
                    ((uint32_t)raw[4] << 8) | (uint32_t)raw[5];

  data->humidity_rh =
      ((float)humidity_raw / AHT20_RAW_FULL_SCALE) * 100.0f;
  data->temperature_c =
      ((float)temperature_raw / AHT20_RAW_FULL_SCALE) * 200.0f - 50.0f;
  data->valid = true;

  return AHT20_OK;
}
