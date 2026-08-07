#ifndef STM32TOOLS_SPI_DISPLAY_BUS_H
#define STM32TOOLS_SPI_DISPLAY_BUS_H

#include <stddef.h>
#include <stdint.h>

typedef struct SPI_DisplayBus SPI_DisplayBus;

typedef int (*SPI_DisplayTransfer)(const SPI_DisplayBus *bus,
                                   const uint8_t *data, size_t length,
                                   uint8_t use_dma);
typedef void (*SPI_DisplayControl)(const SPI_DisplayBus *bus, uint8_t active);
typedef void (*SPI_DisplayDelay)(uint32_t delay_ms);

typedef struct {
  uint8_t command;
  const uint8_t *data;
  uint16_t data_length;
  uint16_t delay_ms;
} SPI_DisplayCommand;

struct SPI_DisplayBus {
  void *spi_handle;
  void *cs_port;
  void *dc_port;
  uint16_t cs_pin;
  uint16_t dc_pin;
  uint16_t dma_threshold;
  uint8_t use_dma;
  SPI_DisplayTransfer transfer;
  SPI_DisplayControl select;
  SPI_DisplayControl data_mode;
};

int SPI_DisplayWriteCommand(const SPI_DisplayBus *bus, uint8_t command);
int SPI_DisplayWriteData(const SPI_DisplayBus *bus, const uint8_t *data,
                         size_t length);
/* Returns 0 on success, negative on error. */
int SPI_DisplayRunSequence(const SPI_DisplayBus *bus,
                           const SPI_DisplayCommand *commands, size_t count,
                           SPI_DisplayDelay delay);

void SPI_DisplayBusInitSTM32(SPI_DisplayBus *bus, void *spi_handle,
                             void *cs_port, uint16_t cs_pin, void *dc_port,
                             uint16_t dc_pin, uint8_t use_dma);

#endif /* STM32TOOLS_SPI_DISPLAY_BUS_H */
