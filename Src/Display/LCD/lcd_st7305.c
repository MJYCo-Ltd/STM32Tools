#ifndef STM32TOOLS_DISPLAY_BACKEND_ST7305
#error "Compile lcd_st7305.c only with STM32TOOLS_DISPLAY_BACKEND_ST7305"
#endif

#include "Display/LCD/lcd_st7305.h"
#include "Display/Graphics.h"

#include <stdint.h>
#include <string.h>

#define ST7305_SWRESET 0x01U
#define ST7305_INV_OFF 0x20U
#define ST7305_INV_ON  0x21U
#define ST7305_CASET   0x2AU
#define ST7305_RASET   0x2BU
#define ST7305_RAMWR   0x2CU
#define ST7305_TE_OFF  0x34U
#define ST7305_TE_ON   0x35U

static ST7305_Binding s_binding;
static ROTATION s_rotation;
static uint8_t s_bound;
static uint8_t s_initialized;

/* Initialization uses this path before publishing the ready state. */
static ST7305_Status ST7305_RefreshAreaInternal(uint16_t x, uint16_t y,
                                               uint16_t width, uint16_t height);

static uint8_t ST7305_ProfileValid(const ST7305_PanelProfile *panel)
{
  uint32_t final_column;
  uint32_t final_row;

  if ((panel == NULL) || (panel->width == 0U) || (panel->height == 0U) ||
      (panel->init_sequence == NULL) || (panel->init_sequence_count == 0U)) {
    return 0U;
  }

  final_column = (uint32_t)panel->column_offset +
                 ((uint32_t)panel->width - 1U) / 12U;
  final_row = (uint32_t)panel->row_offset +
              ((uint32_t)panel->height - 1U) / 2U;
  if ((final_column > UINT8_MAX) || (final_row > UINT8_MAX) ||
      (panel->column_end < panel->column_offset)) {
    return 0U;
  }
  return 1U;
}

size_t ST7305_FramebufferSize(const ST7305_PanelProfile *panel)
{
  size_t row_bytes;

  if (ST7305_ProfileValid(panel) == 0U) {
    return 0U;
  }
  row_bytes = ((size_t)panel->width + 7U) / 8U;
  if ((row_bytes != 0U) && ((size_t)panel->height > (SIZE_MAX / row_bytes))) {
    return 0U;
  }
  return row_bytes * (size_t)panel->height;
}

size_t ST7305_LineBufferSize(const ST7305_PanelProfile *panel)
{
  size_t address_columns;

  if (ST7305_ProfileValid(panel) == 0U) {
    return 0U;
  }
  address_columns = ((size_t)panel->width + 11U) / 12U;
  if (address_columns > (SIZE_MAX / 3U)) {
    return 0U;
  }
  return address_columns * 3U;
}

static uint8_t ST7305_BindingValid(const ST7305_Binding *binding)
{
  size_t framebuffer_size;
  size_t line_buffer_size;

  if ((binding == NULL) || (binding->bus == NULL) ||
      (binding->bus->transfer == NULL) || (binding->bus->data_mode == NULL) ||
      (binding->panel == NULL) || (binding->delay_ms == NULL) ||
      (binding->framebuffer == NULL) || (binding->line_buffer == NULL) ||
      ((unsigned int)binding->rotation > (unsigned int)ROTATION_270)) {
    return 0U;
  }

  framebuffer_size = ST7305_FramebufferSize(binding->panel);
  line_buffer_size = ST7305_LineBufferSize(binding->panel);
  if ((framebuffer_size == 0U) || (line_buffer_size == 0U) ||
      (binding->framebuffer_size < framebuffer_size) ||
      (binding->line_buffer_size < line_buffer_size)) {
    return 0U;
  }
  return 1U;
}

ST7305_Status LCD_ST7305_Bind(const ST7305_Binding *binding)
{
  if (ST7305_BindingValid(binding) == 0U) {
    return ST7305_ERR_PARAM;
  }

  s_binding = *binding;
  s_rotation = binding->rotation;
  s_initialized = 0U;
  s_bound = 1U;
  return ST7305_OK;
}

void LCD_ST7305_Unbind(void)
{
  memset(&s_binding, 0, sizeof(s_binding));
  s_rotation = NO_ROTATION;
  s_initialized = 0U;
  s_bound = 0U;
}

uint8_t LCD_ST7305_IsBound(void)
{
  return s_bound;
}

uint8_t LCD_ST7305_IsReady(void)
{
  return (s_bound != 0U && s_initialized != 0U) ? 1U : 0U;
}

static void ST7305_Delay(uint32_t delay_ms)
{
  if ((s_bound != 0U) && (s_binding.delay_ms != NULL)) {
    s_binding.delay_ms(s_binding.io_context, delay_ms);
  }
}

static ST7305_Status ST7305_WriteCommand(uint8_t command)
{
  if (s_bound == 0U) {
    return ST7305_ERR_STATE;
  }
  return (SPI_DisplayWriteCommand(s_binding.bus, command) == 0)
             ? ST7305_OK
             : ST7305_ERR_IO;
}

static ST7305_Status ST7305_WriteData(const uint8_t *data, size_t length)
{
  if (s_bound == 0U) {
    return ST7305_ERR_STATE;
  }
  if ((data == NULL) || (length == 0U)) {
    return ST7305_ERR_PARAM;
  }
  return (SPI_DisplayWriteData(s_binding.bus, data, length) == 0)
             ? ST7305_OK
             : ST7305_ERR_IO;
}

static ST7305_Status ST7305_Send(uint8_t command, const uint8_t *data,
                                 size_t length)
{
  ST7305_Status status = ST7305_WriteCommand(command);

  if ((status == ST7305_OK) && (data != NULL) && (length != 0U)) {
    status = ST7305_WriteData(data, length);
  }
  return status;
}

static size_t ST7305_RowBytes(void)
{
  return ((size_t)s_binding.panel->width + 7U) / 8U;
}

static size_t ST7305_AddressColumns(void)
{
  return ((size_t)s_binding.panel->width + 11U) / 12U;
}

static uint8_t ST7305_GetPixel(uint16_t x, uint16_t y)
{
  const size_t row_bytes = ST7305_RowBytes();

  if ((x >= s_binding.panel->width) || (y >= s_binding.panel->height)) {
    return 0U;
  }
  return (uint8_t)((s_binding.framebuffer[(size_t)y * row_bytes + (x >> 3U)] >>
                    (7U - (x & 7U))) &
                   1U);
}

/* One ST7305 GRAM address represents 12x2 pixels; each byte interleaves 4x2. */
static uint8_t ST7305_Pack4x2(uint16_t x, uint16_t y)
{
  uint8_t value = 0U;
  uint8_t column;

  for (column = 0U; column < 4U; ++column) {
    value |= (uint8_t)(ST7305_GetPixel((uint16_t)(x + column), y)
                       << (7U - column * 2U));
    value |= (uint8_t)(ST7305_GetPixel((uint16_t)(x + column),
                                      (uint16_t)(y + 1U))
                       << (6U - column * 2U));
  }
  return value;
}

static ST7305_Status ST7305_ResetInternal(void)
{
  /* A hardware or software reset invalidates the controller configuration,
   * including when the reset command itself fails. */
  s_initialized = 0U;
  if (s_bound == 0U) {
    return ST7305_ERR_STATE;
  }

  if (s_binding.reset != NULL) {
    s_binding.reset(s_binding.io_context, 1U);
    ST7305_Delay(10U);
    s_binding.reset(s_binding.io_context, 0U);
    ST7305_Delay(100U);
    return ST7305_OK;
  }

  if (ST7305_WriteCommand(ST7305_SWRESET) != ST7305_OK) {
    return ST7305_ERR_IO;
  }
  ST7305_Delay(100U);
  return ST7305_OK;
}

ST7305_Status LCD_ST7305_Reset(void)
{
  return ST7305_ResetInternal();
}

void LCD_Reset(void)
{
  (void)LCD_ST7305_Reset();
}

ST7305_Status LCD_ST7305_Initialize(void)
{
  ST7305_Status status;
  int sequence_status;

  if (s_bound == 0U) {
    return ST7305_ERR_STATE;
  }
  if (s_initialized != 0U) {
    return ST7305_OK;
  }

  memset(s_binding.framebuffer, 0,
         ST7305_FramebufferSize(s_binding.panel));
  memset(s_binding.line_buffer, 0,
         ST7305_LineBufferSize(s_binding.panel));

  status = ST7305_ResetInternal();
  if (status != ST7305_OK) {
    return status;
  }

  sequence_status = SPI_DisplayRunSequence(
      s_binding.bus, s_binding.panel->init_sequence,
      s_binding.panel->init_sequence_count, ST7305_Delay);
  if (sequence_status != 0) {
    return ST7305_ERR_IO;
  }

  status = ST7305_RefreshAreaInternal(
      0U, 0U,
      (s_rotation == ROTATION_90 || s_rotation == ROTATION_270)
          ? s_binding.panel->height : s_binding.panel->width,
      (s_rotation == ROTATION_90 || s_rotation == ROTATION_270)
          ? s_binding.panel->width : s_binding.panel->height);
  if (status != ST7305_OK) {
    return status;
  }

  /* The caller may observe ready only after the initial frame was sent. */
  s_initialized = 1U;
  return ST7305_OK;
}

void LCD_Init(void)
{
  (void)LCD_ST7305_Initialize();
}

void LCD_SetRotation(ROTATION rotation)
{
  if ((unsigned int)rotation <= (unsigned int)ROTATION_270) {
    s_rotation = rotation;
  }
}

static ST7305_Status ST7305_SetAddressWindow(uint16_t x0, uint16_t y0,
                                             uint16_t x1, uint16_t y1)
{
  uint8_t columns[2];
  uint8_t rows[2];
  const ST7305_PanelProfile *panel;
  ST7305_Status status;

  if (s_bound == 0U) {
    return ST7305_ERR_STATE;
  }
  panel = s_binding.panel;
  if ((x0 >= panel->width) || (y0 >= panel->height) || (x0 > x1) ||
      (y0 > y1)) {
    return ST7305_ERR_PARAM;
  }
  if (x1 >= panel->width) {
    x1 = panel->width - 1U;
  }
  if (y1 >= panel->height) {
    y1 = panel->height - 1U;
  }

  columns[0] = (uint8_t)(panel->column_offset + x0 / 12U);
  columns[1] = (x1 == (panel->width - 1U))
                   ? panel->column_end
                   : (uint8_t)(panel->column_offset + x1 / 12U);
  rows[0] = (uint8_t)(panel->row_offset + y0 / 2U);
  rows[1] = (uint8_t)(panel->row_offset + y1 / 2U);

  status = ST7305_Send(ST7305_CASET, columns, sizeof(columns));
  if (status == ST7305_OK) {
    status = ST7305_Send(ST7305_RASET, rows, sizeof(rows));
  }
  if (status == ST7305_OK) {
    status = ST7305_WriteCommand(ST7305_RAMWR);
  }
  return status;
}

void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1,
                          uint16_t y1)
{
  (void)ST7305_SetAddressWindow(x0, y0, x1, y1);
}

ST7305_Status LCD_ST7305_Refresh(void)
{
  const ST7305_PanelProfile *panel;

  if (LCD_ST7305_IsReady() == 0U) {
    return ST7305_ERR_STATE;
  }
  panel = s_binding.panel;
  return LCD_ST7305_RefreshArea(
      0U, 0U,
      (s_rotation == ROTATION_90 || s_rotation == ROTATION_270)
          ? panel->height : panel->width,
      (s_rotation == ROTATION_90 || s_rotation == ROTATION_270)
          ? panel->width : panel->height);
}

void LCD_Refresh(void)
{
  (void)LCD_ST7305_Refresh();
}

ST7305_Status LCD_ST7305_RefreshArea(uint16_t x, uint16_t y, uint16_t width,
                                     uint16_t height)
{
  ST7305_Status status;
  if (LCD_ST7305_IsReady() == 0U) {
    return ST7305_ERR_STATE;
  }
  status = ST7305_RefreshAreaInternal(x, y, width, height);
  if (status == ST7305_ERR_IO) {
    /* A partial/failed transfer requires explicit reinitialization. */
    s_initialized = 0U;
  }
  return status;
}

void LCD_RefreshArea(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  (void)LCD_ST7305_RefreshArea(x, y, width, height);
}

static ST7305_Status ST7305_RefreshAreaInternal(uint16_t x, uint16_t y,
                                               uint16_t width, uint16_t height)
{
  const ST7305_PanelProfile *panel;
  const size_t address_columns =
      (s_bound != 0U) ? ST7305_AddressColumns() : 0U;
  uint16_t logical_width;
  uint16_t logical_height;
  uint16_t x1;
  uint16_t y1;
  uint16_t native_y0;
  uint16_t native_y1;
  uint16_t row;
  size_t address_column;

  if (s_bound == 0U) {
    return ST7305_ERR_STATE;
  }
  panel = s_binding.panel;
  logical_width = ((s_rotation == ROTATION_90) ||
                   (s_rotation == ROTATION_270))
                      ? panel->height
                      : panel->width;
  logical_height = ((s_rotation == ROTATION_90) ||
                    (s_rotation == ROTATION_270))
                       ? panel->width
                       : panel->height;
  if ((width == 0U) || (height == 0U) || (x >= logical_width) ||
      (y >= logical_height)) {
    return ST7305_ERR_PARAM;
  }

  x1 = (uint16_t)(x + width - 1U);
  y1 = (uint16_t)(y + height - 1U);
  if ((x1 < x) || (x1 >= logical_width)) {
    x1 = (uint16_t)(logical_width - 1U);
  }
  if ((y1 < y) || (y1 >= logical_height)) {
    y1 = (uint16_t)(logical_height - 1U);
  }

  switch (s_rotation) {
  case ROTATION_90:
    native_y0 = x;
    native_y1 = x1;
    break;
  case ROTATION_180:
    native_y0 = (uint16_t)(panel->height - 1U - y1);
    native_y1 = (uint16_t)(panel->height - 1U - y);
    break;
  case ROTATION_270:
    native_y0 = (uint16_t)(panel->height - 1U - x1);
    native_y1 = (uint16_t)(panel->height - 1U - x);
    break;
  case NO_ROTATION:
  default:
    native_y0 = y;
    native_y1 = y1;
    break;
  }

  native_y0 &= (uint16_t)~1U;
  native_y1 |= 1U;
  if (native_y1 >= panel->height) {
    native_y1 = panel->height - 1U;
  }

  for (row = native_y0; row <= native_y1; row += 2U) {
    size_t out = 0U;

    /* This panel is reliable only with a full column window; crop rows only. */
    for (address_column = 0U; address_column < address_columns;
         ++address_column) {
      const uint16_t column_x = (uint16_t)(address_column * 12U);
      s_binding.line_buffer[out++] = ST7305_Pack4x2(column_x, row);
      s_binding.line_buffer[out++] =
          ST7305_Pack4x2((uint16_t)(column_x + 4U), row);
      s_binding.line_buffer[out++] =
          ST7305_Pack4x2((uint16_t)(column_x + 8U), row);
    }
    if (ST7305_SetAddressWindow(0U, row, panel->width - 1U,
                                (uint16_t)(row + 1U)) != ST7305_OK) {
      return ST7305_ERR_IO;
    }
    if (ST7305_WriteData(s_binding.line_buffer, out) != ST7305_OK) {
      return ST7305_ERR_IO;
    }
  }
  return ST7305_OK;
}

void DrawPixel(const Pixel *pixel)
{
  const ST7305_PanelProfile *panel;
  const size_t row_bytes = (s_bound != 0U) ? ST7305_RowBytes() : 0U;
  uint16_t x;
  uint16_t y;
  uint16_t native_x;
  uint16_t native_y;
  uint8_t mask;
  uint8_t is_dark;

  if ((s_bound == 0U) || (pixel == NULL)) {
    return;
  }
  panel = s_binding.panel;
  x = pixel->x;
  y = pixel->y;
  if ((s_rotation == ROTATION_90) || (s_rotation == ROTATION_270)) {
    if ((x >= panel->height) || (y >= panel->width)) {
      return;
    }
  } else if ((x >= panel->width) || (y >= panel->height)) {
    return;
  }

  switch (s_rotation) {
  case ROTATION_90:
    native_x = (uint16_t)(panel->width - 1U - y);
    native_y = x;
    break;
  case ROTATION_180:
    native_x = (uint16_t)(panel->width - 1U - x);
    native_y = (uint16_t)(panel->height - 1U - y);
    break;
  case ROTATION_270:
    native_x = y;
    native_y = (uint16_t)(panel->height - 1U - x);
    break;
  case NO_ROTATION:
  default:
    native_x = x;
    native_y = y;
    break;
  }

  is_dark = (uint8_t)((77U * pixel->color.uRed +
                       150U * pixel->color.uGreen +
                       29U * pixel->color.uBlue) < 32768U);
  mask = (uint8_t)(1U << (7U - (native_x & 7U)));
  if (is_dark != 0U) {
    s_binding.framebuffer[(size_t)native_y * row_bytes + (native_x >> 3U)] |=
        mask;
  } else {
    s_binding.framebuffer[(size_t)native_y * row_bytes + (native_x >> 3U)] &=
        (uint8_t)~mask;
  }
}

void LCD_InvertColors(uint8_t invert)
{
  if (s_bound != 0U) {
    (void)ST7305_WriteCommand((invert != 0U) ? ST7305_INV_ON
                                             : ST7305_INV_OFF);
  }
}

void LCD_TearEffect(uint8_t tear)
{
  if (s_bound == 0U) {
    return;
  }
  if (tear != 0U) {
    const uint8_t mode = 0x00U;
    (void)ST7305_Send(ST7305_TE_ON, &mode, 1U);
  } else {
    (void)ST7305_WriteCommand(ST7305_TE_OFF);
  }
}
