#include "Bus/i2c_bus.h"

#include "Common.h"

I2C_BusResult I2C_BusTransmit(const I2C_Bus *bus, uint8_t address7,
                              const uint8_t *data, size_t length) {
  if ((bus == NULL) || (bus->transmit == NULL) || (data == NULL) ||
      (length == 0U)) {
    return I2C_BUS_INVALID_ARGUMENT;
  }
  return bus->transmit(bus->handle, address7, (uint8_t *)data, length,
                       bus->timeout_ms);
}

I2C_BusResult I2C_BusReceive(const I2C_Bus *bus, uint8_t address7,
                             uint8_t *data, size_t length) {
  if ((bus == NULL) || (bus->receive == NULL) || (data == NULL) ||
      (length == 0U)) {
    return I2C_BUS_INVALID_ARGUMENT;
  }
  return bus->receive(bus->handle, address7, data, length, bus->timeout_ms);
}

I2C_BusResult I2C_BusWriteBE16(const I2C_Bus *bus, uint8_t address7,
                               uint8_t reg, uint16_t value) {
  uint8_t packet[3];
  packet[0] = reg;
  WriteBE16(packet + 1, value);
  return I2C_BusTransmit(bus, address7, packet, sizeof(packet));
}

I2C_BusResult I2C_BusReadBE16(const I2C_Bus *bus, uint8_t address7,
                              uint8_t reg, uint16_t *value) {
  uint8_t data[2];
  I2C_BusResult result;

  if (value == NULL) {
    return I2C_BUS_INVALID_ARGUMENT;
  }
  result = I2C_BusTransmit(bus, address7, &reg, 1U);
  if (result != I2C_BUS_OK) {
    return result;
  }
  result = I2C_BusReceive(bus, address7, data, sizeof(data));
  if (result == I2C_BUS_OK) {
    *value = ReadBE16(data);
  }
  return result;
}
