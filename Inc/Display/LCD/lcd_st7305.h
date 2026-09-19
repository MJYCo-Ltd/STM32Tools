#ifndef STM32TOOLS_LCD_ST7305_H
#define STM32TOOLS_LCD_ST7305_H

#include <stddef.h>
#include <stdint.h>

#include "Display/LCD/lcd.h"
#include "Display/spi_display_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ST7305_OK = 0,
  ST7305_ERR_PARAM = -1,
  ST7305_ERR_STATE = -2,
  ST7305_ERR_IO = -3
} ST7305_Status;

typedef void (*ST7305_ResetControl)(void *context, uint8_t asserted);
typedef void (*ST7305_DelayMs)(void *context, uint32_t delay_ms);

typedef struct {
  uint16_t width;
  uint16_t height;
  uint8_t column_offset;
  uint8_t column_end;
  uint8_t row_offset;
  const SPI_DisplayCommand *init_sequence;
  size_t init_sequence_count;
} ST7305_PanelProfile;

typedef struct {
  const SPI_DisplayBus *bus;
  const ST7305_PanelProfile *panel;
  ST7305_ResetControl reset;
  ST7305_DelayMs delay_ms;
  void *io_context;
  uint8_t *framebuffer;
  size_t framebuffer_size;
  uint8_t *line_buffer;
  size_t line_buffer_size;
  ROTATION rotation;
} ST7305_Binding;

#define ST7305_FD042MN_ZF21_H06_B_WIDTH 300U
#define ST7305_FD042MN_ZF21_H06_B_HEIGHT 400U
#define ST7305_FD042MN_ZF21_H06_B_FRAMEBUFFER_SIZE \
  ((((ST7305_FD042MN_ZF21_H06_B_WIDTH) + 7U) / 8U) * \
   ST7305_FD042MN_ZF21_H06_B_HEIGHT)
#define ST7305_FD042MN_ZF21_H06_B_LINE_BUFFER_SIZE \
  ((((ST7305_FD042MN_ZF21_H06_B_WIDTH) + 11U) / 12U) * 3U)

extern const ST7305_PanelProfile ST7305_PANEL_FD042MN_ZF21_H06_B;

size_t ST7305_FramebufferSize(const ST7305_PanelProfile *panel);
size_t ST7305_LineBufferSize(const ST7305_PanelProfile *panel);

/** Bind the singleton LCD compatibility facade to explicit board resources. */
ST7305_Status LCD_ST7305_Bind(const ST7305_Binding *binding);
void LCD_ST7305_Unbind(void);
uint8_t LCD_ST7305_IsBound(void);

/** Initialize the bound panel and perform the initial framebuffer refresh. */
ST7305_Status LCD_ST7305_Initialize(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_LCD_ST7305_H */
