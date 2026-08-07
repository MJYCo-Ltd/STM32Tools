#ifndef __SPI_DISPLAY_H__
#define __SPI_DISPLAY_H__

#include <stddef.h>
#include <stdint.h>

#include "Display/spi_display_bus.h"

/* Compatibility facade for existing display drivers. New drivers should keep
 * SPI_DisplayBus in their device context and call SPI_DisplayWrite* directly. */
static SPI_DisplayBus s_spi_display_bus;
static uint8_t s_spi_display_bus_ready;

static inline SPI_DisplayBus *SPI_GetDisplayBus(void) {
  if (s_spi_display_bus_ready == 0U) {
#ifdef CFG_NO_CS
    void *cs_port = NULL;
    uint16_t cs_pin = 0U;
#else
    void *cs_port = (void *)ST_DISPLAY_CS_PORT;
    uint16_t cs_pin = ST_DISPLAY_CS_PIN;
#endif
#ifdef USE_DMA
    uint8_t use_dma = 1U;
#else
    uint8_t use_dma = 0U;
#endif
    SPI_DisplayBusInitSTM32(&s_spi_display_bus, &DISPLAY_SPI_PORT, cs_port,
                            cs_pin, (void *)ST_DISPLAY_DC_PORT,
                            ST_DISPLAY_DC_PIN, use_dma);
    s_spi_display_bus_ready = 1U;
  }
  return &s_spi_display_bus;
}

static inline void SPI_SendCommand(uint8_t command) {
  (void)SPI_DisplayWriteCommand(SPI_GetDisplayBus(), command);
}

static inline void SPI_SendData(uint8_t data) {
  (void)SPI_DisplayWriteData(SPI_GetDisplayBus(), &data, 1U);
}

static inline void SPI_SendBuffer(const uint8_t *buffer, size_t length) {
  (void)SPI_DisplayWriteData(SPI_GetDisplayBus(), buffer, length);
}

#endif /* __SPI_DISPLAY_H__ */
