#include "Display/spi_display_bus.h"

static int SPI_DisplayWrite(const SPI_DisplayBus *bus, const uint8_t *data,
                            size_t length, uint8_t data_mode) {
  int result;
  uint8_t dma;

  if ((bus == NULL) || (bus->transfer == NULL) || (bus->data_mode == NULL) ||
      (data == NULL) || (length == 0U)) {
    return 0;
  }
  dma = ((bus->use_dma != 0U) && (length >= bus->dma_threshold)) ? 1U : 0U;
  if (bus->select != NULL) {
    bus->select(bus, 1U);
  }
  bus->data_mode(bus, data_mode);
  result = bus->transfer(bus, data, length, dma);
  if (bus->select != NULL) {
    bus->select(bus, 0U);
  }
  return result;
}

int SPI_DisplayWriteCommand(const SPI_DisplayBus *bus, uint8_t command) {
  return SPI_DisplayWrite(bus, &command, 1U, 0U);
}

int SPI_DisplayWriteData(const SPI_DisplayBus *bus, const uint8_t *data,
                         size_t length) {
  return SPI_DisplayWrite(bus, data, length, 1U);
}
