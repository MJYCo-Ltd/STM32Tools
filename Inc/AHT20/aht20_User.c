#include "AHT20/aht20.h"
#include "main.h"

extern I2C_HandleTypeDef hi2c1;

#ifndef AHT20_I2C_TIMEOUT_MS
#define AHT20_I2C_TIMEOUT_MS 100U
#endif

__attribute__((weak)) AHT20_Status AHT20_I2C_Transmit(uint8_t addr7,
                                                      const uint8_t *data,
                                                      uint16_t length)
{
  HAL_StatusTypeDef status;

  if ((data == NULL) || (length == 0U)) {
    return AHT20_ERR_PARAM;
  }

  status = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(addr7 << 1),
                                   (uint8_t *)data, length,
                                   AHT20_I2C_TIMEOUT_MS);
  return (status == HAL_OK) ? AHT20_OK : AHT20_ERR_WRITE_I2C;
}

__attribute__((weak)) AHT20_Status AHT20_I2C_Receive(uint8_t addr7,
                                                     uint8_t *data,
                                                     uint16_t length)
{
  HAL_StatusTypeDef status;

  if ((data == NULL) || (length == 0U)) {
    return AHT20_ERR_PARAM;
  }

  status = HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(addr7 << 1), data, length,
                                  AHT20_I2C_TIMEOUT_MS);
  return (status == HAL_OK) ? AHT20_OK : AHT20_ERR_READ_I2C;
}
