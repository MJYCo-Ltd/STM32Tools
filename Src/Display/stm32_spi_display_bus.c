#include "Display/spi_display_bus.h"

#include "Base.h"
#include "main.h"

static void STM32_Select(const SPI_DisplayBus *bus, uint8_t active) {
  if (bus->cs_port != NULL) {
    HAL_GPIO_WritePin((GPIO_TypeDef *)bus->cs_port, bus->cs_pin,
                      (active != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
  }
}

static void STM32_DataMode(const SPI_DisplayBus *bus, uint8_t data_mode) {
  HAL_GPIO_WritePin((GPIO_TypeDef *)bus->dc_port, bus->dc_pin,
                    (data_mode != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static int STM32_Transfer(const SPI_DisplayBus *bus, const uint8_t *data,
                          size_t length, uint8_t use_dma) {
  SPI_HandleTypeDef *spi = (SPI_HandleTypeDef *)bus->spi_handle;

  while (length > 0U) {
    uint16_t chunk = (length > UINT16_MAX) ? UINT16_MAX : (uint16_t)length;
    HAL_StatusTypeDef status;

    if (use_dma != 0U) {
      status = HAL_SPI_Transmit_DMA(spi, (uint8_t *)data, chunk);
      while ((status == HAL_OK) && (HAL_SPI_GetState(spi) != HAL_SPI_STATE_READY)) {
        YTY_DELAY_MS(1U);
      }
    } else {
      status = HAL_SPI_Transmit(spi, (uint8_t *)data, chunk, HAL_MAX_DELAY);
    }
    if (status != HAL_OK) {
      return -1;
    }
    data += chunk;
    length -= chunk;
  }
  return 0;
}

void SPI_DisplayBusInitSTM32(SPI_DisplayBus *bus, void *spi_handle,
                             void *cs_port, uint16_t cs_pin, void *dc_port,
                             uint16_t dc_pin, uint8_t use_dma) {
  if (bus != NULL) {
    bus->spi_handle = spi_handle;
    bus->cs_port = cs_port;
    bus->dc_port = dc_port;
    bus->cs_pin = cs_pin;
    bus->dc_pin = dc_pin;
    bus->dma_threshold = 16U;
    bus->use_dma = use_dma;
    bus->transfer = STM32_Transfer;
    bus->select = STM32_Select;
    bus->data_mode = STM32_DataMode;
  }
}
