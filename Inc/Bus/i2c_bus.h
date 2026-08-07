#ifndef STM32TOOLS_I2C_BUS_H
#define STM32TOOLS_I2C_BUS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  I2C_BUS_OK = 0,
  I2C_BUS_INVALID_ARGUMENT,
  I2C_BUS_TIMEOUT,
  I2C_BUS_IO_ERROR
} I2C_BusResult;

typedef I2C_BusResult (*I2C_BusTransfer)(void *handle, uint8_t address7,
                                        uint8_t *data, size_t length,
                                        uint32_t timeout_ms);

typedef struct {
  void *handle;
  I2C_BusTransfer transmit;
  I2C_BusTransfer receive;
  uint32_t timeout_ms;
} I2C_Bus;

I2C_BusResult I2C_BusTransmit(const I2C_Bus *bus, uint8_t address7,
                              const uint8_t *data, size_t length);
I2C_BusResult I2C_BusReceive(const I2C_Bus *bus, uint8_t address7,
                             uint8_t *data, size_t length);
I2C_BusResult I2C_BusWriteBE16(const I2C_Bus *bus, uint8_t address7,
                               uint8_t reg, uint16_t value);
I2C_BusResult I2C_BusReadBE16(const I2C_Bus *bus, uint8_t address7,
                              uint8_t reg, uint16_t *value);

/** Configure a bus for blocking STM32 HAL I2C transfers. */
void I2C_BusInitSTM32(I2C_Bus *bus, void *hal_i2c, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_I2C_BUS_H */
