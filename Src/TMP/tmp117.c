#include "TMP/tmp117.h"

#include <stddef.h>

#include "Base.h"

#define TMP117_REG_TEMP_RES      0x00U
#define TMP117_REG_CONFIGURATION 0x01U
#define TMP117_CFG_MOD0_Pos      10U

static I2C_Bus s_default_bus;
static const I2C_Bus *s_bus;

#if defined(PLATFORM_STM32) || defined(USE_HAL_DRIVER)
#include "main.h"
extern I2C_HandleTypeDef hi2c1;
#endif

static const I2C_Bus *TMP117_GetBus(void) {
#if defined(PLATFORM_STM32) || defined(USE_HAL_DRIVER)
  if (s_bus == NULL) {
    I2C_BusInitSTM32(&s_default_bus, &hi2c1, HAL_MAX_DELAY);
    s_bus = &s_default_bus;
  }
#endif
  return s_bus;
}

void TMP117_SetBus(const I2C_Bus *bus) { s_bus = bus; }

static TMP117_Status TMP117_MapResult(I2C_BusResult result, uint8_t reading) {
  if (result == I2C_BUS_OK) {
    return TMP117_OK;
  }
  if (result == I2C_BUS_TIMEOUT) {
    return TMP117_ERR_TIMEOUT;
  }
  return (reading != 0U) ? TMP117_ERR_READ_I2C : TMP117_ERR_WRITE_I2C;
}

static TMP117_Status TMP117_ReadRegister(uint8_t address7, uint8_t reg,
                                         uint16_t *value) {
  return TMP117_MapResult(
      I2C_BusReadBE16(TMP117_GetBus(), address7, reg, value), 1U);
}

static TMP117_Status TMP117_WriteRegister(uint8_t address7, uint8_t reg,
                                          uint16_t value) {
  return TMP117_MapResult(
      I2C_BusWriteBE16(TMP117_GetBus(), address7, reg, value), 0U);
}

TMP117_Status TMP117_GetTemperature(uint8_t address7, TMP117_Temp *temp) {
  TMP117_Status status;

  if (temp == NULL) {
    return TMP117_ERR_RANGE;
  }
  status = TMP117_ReadRegister(address7, TMP117_REG_TEMP_RES, &temp->uValue);
  if (status != TMP117_OK) {
    return status;
  }
  temp->value = ((int16_t)temp->uValue) * 0.0078125f;
  temp->valid = (temp->value >= 35.0f) && (temp->value <= 42.0f);
  return TMP117_OK;
}

TMP117_Status TMP117_SetWorkMode(uint8_t address7, TMP117_Mode work_mode) {
  uint16_t configuration;
  TMP117_Status status;

  if ((work_mode != TMP117_MODE_CONTINUOUS) &&
      (work_mode != TMP117_MODE_SHUTDOWN) &&
      (work_mode != TMP117_MODE_ONE_SHOT)) {
    return TMP117_ERR_RANGE;
  }
  status = TMP117_ReadRegister(address7, TMP117_REG_CONFIGURATION,
                               &configuration);
  if (status != TMP117_OK) {
    return status;
  }
  configuration &= (uint16_t)~(3U << TMP117_CFG_MOD0_Pos);
  configuration |= (uint16_t)work_mode << TMP117_CFG_MOD0_Pos;
  return TMP117_WriteRegister(address7, TMP117_REG_CONFIGURATION,
                              configuration);
}
