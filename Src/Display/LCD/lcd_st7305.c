#ifndef STM32TOOLS_DISPLAY_BACKEND_ST7305
#error "Compile lcd_st7305.c only with STM32TOOLS_DISPLAY_BACKEND_ST7305"
#endif

#include <cmsis_os.h>
#include <string.h>

#include <Display/LCD/lcd.h>
#include <Display/LCD/lcd_st7305_config.h>
#include <Display/SPIDisplay.h>

/* ST7305 commands. */
#define ST7305_SWRESET   0x01U
#define ST7305_SLPIN     0x10U
#define ST7305_SLPOUT    0x11U
#define ST7305_INVOFF    0x20U
#define ST7305_INVON     0x21U
#define ST7305_DISPOFF   0x28U
#define ST7305_DISPON    0x29U
#define ST7305_CASET     0x2AU
#define ST7305_RASET     0x2BU
#define ST7305_RAMWR     0x2CU
#define ST7305_TEON      0x35U
#define ST7305_TEOFF     0x34U
#define ST7305_MADCTL    0x36U
#define ST7305_HPM       0x38U
#define ST7305_DTFORM    0x3AU
#define ST7305_GATESET   0xB0U
#define ST7305_FRAMERATE 0xB2U
#define ST7305_HPM_EQ    0xB3U
#define ST7305_LPM_EQ    0xB4U
#define ST7305_SOURCE_EQ 0xB7U
#define ST7305_PNLSET    0xB8U
#define ST7305_GAMAMS    0xB9U
#define ST7305_CLEAR_RAM 0xBBU
#define ST7305_VG        0xC0U
#define ST7305_VSHP      0xC1U
#define ST7305_VSLP      0xC2U
#define ST7305_VSHN      0xC4U
#define ST7305_VSLN      0xC5U
#define ST7305_VSHLSEL   0xC9U
#define ST7305_AUTOPD    0xD0U
#define ST7305_BOOSTER   0xD1U
#define ST7305_NVM_LOAD  0xD6U
#define ST7305_OSC       0xD8U

#define ST7305_ROW_BYTES       ((ST7305_WIDTH + 7U) / 8U)
#define ST7305_BUFFER_SIZE     (ST7305_ROW_BYTES * ST7305_HEIGHT)
#define ST7305_ADDRESS_COLUMNS ((ST7305_WIDTH + 11U) / 12U)
#define ST7305_WIRE_ROW_BYTES  (ST7305_ADDRESS_COLUMNS * 3U)

static uint8_t lcd_buffer[ST7305_BUFFER_SIZE];
static ROTATION lcd_rotation = ST7305_DEFAULT_ROTATION;

static void ST7305_Delay(uint32_t delay_ms) { osDelay(delay_ms); }

static void ST7305_Send(uint8_t command, const uint8_t *data, size_t length) {
  SPI_SendCommand(command);
  if ((data != NULL) && (length != 0U)) {
    SPI_SendBuffer(data, length);
  }
}

static uint8_t ST7305_GetPixel(uint16_t x, uint16_t y) {
  if ((x >= ST7305_WIDTH) || (y >= ST7305_HEIGHT)) {
    return 0U;
  }
  return (uint8_t)((lcd_buffer[(size_t)y * ST7305_ROW_BYTES + (x >> 3U)] >>
                    (7U - (x & 7U))) &
                   1U);
}

/* 一个 ST7305 GRAM 地址表示 12x2 像素，每字节交错存放 4 列的两行。 */
static uint8_t ST7305_Pack4x2(uint16_t x, uint16_t y) {
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

void LCD_Reset(void) {
#ifndef CFG_NO_REST
  ST7305_RST_Clr();
  osDelay(10U);
  ST7305_RST_Set();
  osDelay(100U);
#else
  SPI_SendCommand(ST7305_SWRESET);
  osDelay(100U);
#endif
}

void LCD_Init(void) {
  static const uint8_t nvm_load[] = {ST7305_NVM_LOAD_0,
                                      ST7305_NVM_LOAD_1};
  static const uint8_t booster[] = {0x01U};
  static const uint8_t gate_voltage[] = {ST7305_GATE_VOLTAGE_0,
                                         ST7305_GATE_VOLTAGE_1};
  static const uint8_t vshp[] = ST7305_VSHP_VALUES;
  static const uint8_t vslp[] = ST7305_VSLP_VALUES;
  static const uint8_t vshn[] = ST7305_VSHN_VALUES;
  static const uint8_t vsln[] = ST7305_VSLN_VALUES;
  static const uint8_t oscillator[] = {ST7305_OSC_VALUE, 0xE9U};
  static const uint8_t frame_rate[] = {ST7305_FRAME_RATE};
  static const uint8_t hpm_eq[] = ST7305_HPM_EQ_VALUES;
  static const uint8_t lpm_eq[] = ST7305_LPM_EQ_VALUES;
  static const uint8_t gate_timing[] = ST7305_GATE_TIMING_VALUES;
  static const uint8_t source_eq[] = {0x13U};
  static const uint8_t gate_lines[] = {ST7305_GATE_LINES};
  static const uint8_t vshl_select[] = {0x00U};
  static const uint8_t madctl[] = {ST7305_MADCTL_VALUE};
  static const uint8_t data_format[] = {0x11U};
  static const uint8_t gamma[] = {0x20U};
  static const uint8_t panel[] = {0x29U};
  static const uint8_t tear[] = {0x00U};
  static const uint8_t auto_power_down[] = {0xFFU};
  static const uint8_t clear_ram[] = {0x4FU};
  static const SPI_DisplayCommand init_sequence[] = {
      {ST7305_NVM_LOAD, nvm_load, sizeof(nvm_load), 0U},
      {ST7305_BOOSTER, booster, sizeof(booster), 0U},
      {ST7305_VG, gate_voltage, sizeof(gate_voltage), 0U},
      {ST7305_VSHP, vshp, sizeof(vshp), 0U},
      {ST7305_VSLP, vslp, sizeof(vslp), 0U},
      {ST7305_VSHN, vshn, sizeof(vshn), 0U},
      {ST7305_VSLN, vsln, sizeof(vsln), 0U},
      {ST7305_OSC, oscillator, sizeof(oscillator), 0U},
      {ST7305_FRAMERATE, frame_rate, sizeof(frame_rate), 0U},
      {ST7305_HPM_EQ, hpm_eq, sizeof(hpm_eq), 0U},
      {ST7305_LPM_EQ, lpm_eq, sizeof(lpm_eq), 0U},
      {0x62U, gate_timing, sizeof(gate_timing), 0U},
      {ST7305_SOURCE_EQ, source_eq, sizeof(source_eq), 0U},
      {ST7305_GATESET, gate_lines, sizeof(gate_lines), 0U},
      {ST7305_SLPOUT, NULL, 0U, 120U},
      {ST7305_VSHLSEL, vshl_select, sizeof(vshl_select), 0U},
      {ST7305_MADCTL, madctl, sizeof(madctl), 0U},
      {ST7305_DTFORM, data_format, sizeof(data_format), 0U},
      {ST7305_GAMAMS, gamma, sizeof(gamma), 0U},
      {ST7305_PNLSET, panel, sizeof(panel), 0U},
      {ST7305_TEON, tear, sizeof(tear), 0U},
      {ST7305_AUTOPD, auto_power_down, sizeof(auto_power_down), 0U},
      {ST7305_HPM, NULL, 0U, 0U},
      {ST7305_INVOFF, NULL, 0U, 0U},
      {ST7305_CLEAR_RAM, clear_ram, sizeof(clear_ram), 0U},
      {ST7305_DISPON, NULL, 0U, 100U}};

  memset(lcd_buffer, 0, sizeof(lcd_buffer));
  LCD_Reset();
  (void)SPI_DisplayRunSequence(
      SPI_GetDisplayBus(), init_sequence,
      sizeof(init_sequence) / sizeof(init_sequence[0]), ST7305_Delay);

  LCD_Refresh();
}

void LCD_SetRotation(ROTATION rotation) { lcd_rotation = rotation; }

void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1,
                          uint16_t y1) {
  uint8_t columns[2];
  uint8_t rows[2];

  if ((x0 >= ST7305_WIDTH) || (y0 >= ST7305_HEIGHT) || (x0 > x1) ||
      (y0 > y1)) {
    return;
  }
  if (x1 >= ST7305_WIDTH) {
    x1 = ST7305_WIDTH - 1U;
  }
  if (y1 >= ST7305_HEIGHT) {
    y1 = ST7305_HEIGHT - 1U;
  }
  columns[0] = (uint8_t)(ST7305_COLUMN_OFFSET + x0 / 12U);
  columns[1] = (x1 == (ST7305_WIDTH - 1U))
                   ? ST7305_COLUMN_END
                   : (uint8_t)(ST7305_COLUMN_OFFSET + x1 / 12U);
  rows[0] = (uint8_t)(ST7305_ROW_OFFSET + y0 / 2U);
  rows[1] = (uint8_t)(ST7305_ROW_OFFSET + y1 / 2U);
  ST7305_Send(ST7305_CASET, columns, sizeof(columns));
  ST7305_Send(ST7305_RASET, rows, sizeof(rows));
  SPI_SendCommand(ST7305_RAMWR);
}

void LCD_Refresh(void) {
  LCD_RefreshArea(0U, 0U,
                  (lcd_rotation == ROTATION_90 || lcd_rotation == ROTATION_270)
                      ? ST7305_HEIGHT
                      : ST7305_WIDTH,
                  (lcd_rotation == ROTATION_90 || lcd_rotation == ROTATION_270)
                      ? ST7305_WIDTH
                      : ST7305_HEIGHT);
}

void LCD_RefreshArea(uint16_t x, uint16_t y, uint16_t width,
                     uint16_t height) {
  uint8_t line[ST7305_WIRE_ROW_BYTES];
  uint16_t logical_width;
  uint16_t logical_height;
  uint16_t x1;
  uint16_t y1;
  uint16_t native_y0;
  uint16_t native_y1;
  uint16_t row;
  uint16_t address_column;

  logical_width = ((lcd_rotation == ROTATION_90) ||
                   (lcd_rotation == ROTATION_270))
                      ? ST7305_HEIGHT
                      : ST7305_WIDTH;
  logical_height = ((lcd_rotation == ROTATION_90) ||
                    (lcd_rotation == ROTATION_270))
                       ? ST7305_WIDTH
                       : ST7305_HEIGHT;
  if ((width == 0U) || (height == 0U) || (x >= logical_width) ||
      (y >= logical_height)) {
    return;
  }
  x1 = (uint16_t)(x + width - 1U);
  y1 = (uint16_t)(y + height - 1U);
  if ((x1 < x) || (x1 >= logical_width)) {
    x1 = (uint16_t)(logical_width - 1U);
  }
  if ((y1 < y) || (y1 >= logical_height)) {
    y1 = (uint16_t)(logical_height - 1U);
  }

  switch (lcd_rotation) {
  case ROTATION_90:
    native_y0 = x;
    native_y1 = x1;
    break;
  case ROTATION_180:
    native_y0 = (uint16_t)(ST7305_HEIGHT - 1U - y1);
    native_y1 = (uint16_t)(ST7305_HEIGHT - 1U - y);
    break;
  case ROTATION_270:
    native_y0 = (uint16_t)(ST7305_HEIGHT - 1U - x1);
    native_y1 = (uint16_t)(ST7305_HEIGHT - 1U - x);
    break;
  case NO_ROTATION:
  default:
    native_y0 = y;
    native_y1 = y1;
    break;
  }

  native_y0 &= (uint16_t)~1U;
  native_y1 |= 1U;
  if (native_y1 >= ST7305_HEIGHT) {
    native_y1 = ST7305_HEIGHT - 1U;
  }

  for (row = native_y0; row <= native_y1; row += 2U) {
    size_t out = 0U;
    /* ST7305 300x400 的可靠写法是保持完整列窗口，仅裁剪行地址。
       非零列起始地址在此面板上会从可视区起点写入，造成画面下移/重复。 */
    for (address_column = 0U; address_column < ST7305_ADDRESS_COLUMNS;
         ++address_column) {
      const uint16_t column_x = (uint16_t)(address_column * 12U);
      line[out++] = ST7305_Pack4x2(column_x, row);
      line[out++] = ST7305_Pack4x2((uint16_t)(column_x + 4U), row);
      line[out++] = ST7305_Pack4x2((uint16_t)(column_x + 8U), row);
    }
    LCD_SetAddressWindow(0U, row, ST7305_WIDTH - 1U,
                         (uint16_t)(row + 1U));
    SPI_SendBuffer(line, out);
  }
}

void DrawPixel(const Pixel *pixel) {
  uint16_t x;
  uint16_t y;
  uint16_t native_x;
  uint16_t native_y;
  uint8_t mask;
  uint8_t is_dark;

  if (pixel == NULL) {
    return;
  }
  x = pixel->x;
  y = pixel->y;
  if ((lcd_rotation == ROTATION_90) || (lcd_rotation == ROTATION_270)) {
    if ((x >= ST7305_HEIGHT) || (y >= ST7305_WIDTH)) {
      return;
    }
  } else if ((x >= ST7305_WIDTH) || (y >= ST7305_HEIGHT)) {
      return;
  }

  switch (lcd_rotation) {
  case ROTATION_90:
    native_x = (uint16_t)(ST7305_WIDTH - 1U - y);
    native_y = x;
    break;
  case ROTATION_180:
    native_x = (uint16_t)(ST7305_WIDTH - 1U - x);
    native_y = (uint16_t)(ST7305_HEIGHT - 1U - y);
    break;
  case ROTATION_270:
    native_x = y;
    native_y = (uint16_t)(ST7305_HEIGHT - 1U - x);
    break;
  case NO_ROTATION:
  default:
    native_x = x;
    native_y = y;
    break;
  }

  /* 单色化：感知亮度低于 50% 时显示为黑色。 */
  is_dark = (uint8_t)((77U * pixel->color.uRed +
                       150U * pixel->color.uGreen +
                       29U * pixel->color.uBlue) < 32768U);
  mask = (uint8_t)(1U << (7U - (native_x & 7U)));
  if (is_dark != 0U) {
    lcd_buffer[(size_t)native_y * ST7305_ROW_BYTES + (native_x >> 3U)] |= mask;
  } else {
    lcd_buffer[(size_t)native_y * ST7305_ROW_BYTES + (native_x >> 3U)] &=
        (uint8_t)~mask;
  }
}

void LCD_InvertColors(uint8_t invert) {
  SPI_SendCommand(invert != 0U ? ST7305_INVON : ST7305_INVOFF);
}

void LCD_TearEffect(uint8_t tear) {
  if (tear != 0U) {
    uint8_t mode = 0x00U;
    ST7305_Send(ST7305_TEON, &mode, 1U);
  } else {
    SPI_SendCommand(ST7305_TEOFF);
  }
}
