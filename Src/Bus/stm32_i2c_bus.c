#include "Bus/i2c_bus.h"

#include "main.h"

static I2C_BusResult STM32_Transmit(void *handle, uint8_t address7,
                                    uint8_t *data, size_t length,
                                    uint32_t timeout_ms) {
  HAL_StatusTypeDef status;
  if ((handle == NULL) || (length > UINT16_MAX)) {
    return I2C_BUS_INVALID_ARGUMENT;
  }
  status = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)handle,
                                   (uint16_t)(address7 << 1), data,
                                   (uint16_t)length, timeout_ms);
  return (status == HAL_OK) ? I2C_BUS_OK
         : (status == HAL_TIMEOUT) ? I2C_BUS_TIMEOUT
                                   : I2C_BUS_IO_ERROR;
}

static I2C_BusResult STM32_Receive(void *handle, uint8_t address7,
                                   uint8_t *data, size_t length,
                                   uint32_t timeout_ms) {
  HAL_StatusTypeDef status;
  if ((handle == NULL) || (length > UINT16_MAX)) {
    return I2C_BUS_INVALID_ARGUMENT;
  }
  status = HAL_I2C_Master_Receive((I2C_HandleTypeDef *)handle,
                                  (uint16_t)(address7 << 1), data,
                                  (uint16_t)length, timeout_ms);
  return (status == HAL_OK) ? I2C_BUS_OK
         : (status == HAL_TIMEOUT) ? I2C_BUS_TIMEOUT
                                   : I2C_BUS_IO_ERROR;
}

void I2C_BusInitSTM32(I2C_Bus *bus, void *hal_i2c, uint32_t timeout_ms) {
  if (bus != NULL) {
    bus->handle = hal_i2c;
    bus->transmit = STM32_Transmit;
    bus->receive = STM32_Receive;
    bus->timeout_ms = timeout_ms;
  }
}
