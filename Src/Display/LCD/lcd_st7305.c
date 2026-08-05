#include <cmsis_os.h>
#include <string.h>

#include <Display/LCD/lcd.h>
#include <Display/LCD/lcd_st7305_user.c>
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
static ROTATION lcd_rotation = NO_ROTATION;

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
  static const uint8_t vshp[] = ST7305_VSHP_VALUES;
  static const uint8_t vslp[] = ST7305_VSLP_VALUES;
  static const uint8_t vshn[] = ST7305_VSHN_VALUES;
  static const uint8_t vsln[] = ST7305_VSLN_VALUES;
  static const uint8_t hpm_eq[] = ST7305_HPM_EQ_VALUES;
  static const uint8_t lpm_eq[] = ST7305_LPM_EQ_VALUES;
  static const uint8_t gate_timing[] = ST7305_GATE_TIMING_VALUES;
  uint8_t data[2];

  memset(lcd_buffer, 0, sizeof(lcd_buffer));
  LCD_Reset();

  data[0] = ST7305_NVM_LOAD_0;
  data[1] = ST7305_NVM_LOAD_1;
  ST7305_Send(ST7305_NVM_LOAD, data, 2U);
  data[0] = 0x01U;
  ST7305_Send(ST7305_BOOSTER, data, 1U);
  data[0] = ST7305_GATE_VOLTAGE_0;
  data[1] = ST7305_GATE_VOLTAGE_1;
  ST7305_Send(ST7305_VG, data, 2U);
  ST7305_Send(ST7305_VSHP, vshp, sizeof(vshp));
  ST7305_Send(ST7305_VSLP, vslp, sizeof(vslp));
  ST7305_Send(ST7305_VSHN, vshn, sizeof(vshn));
  ST7305_Send(ST7305_VSLN, vsln, sizeof(vsln));
  data[0] = ST7305_OSC_VALUE;
  data[1] = 0xE9U;
  ST7305_Send(ST7305_OSC, data, 2U);
  data[0] = ST7305_FRAME_RATE;
  ST7305_Send(ST7305_FRAMERATE, data, 1U);
  ST7305_Send(ST7305_HPM_EQ, hpm_eq, sizeof(hpm_eq));
  ST7305_Send(ST7305_LPM_EQ, lpm_eq, sizeof(lpm_eq));
  ST7305_Send(0x62U, gate_timing, sizeof(gate_timing));
  data[0] = 0x13U;
  ST7305_Send(ST7305_SOURCE_EQ, data, 1U);
  data[0] = ST7305_GATE_LINES;
  ST7305_Send(ST7305_GATESET, data, 1U);

  SPI_SendCommand(ST7305_SLPOUT);
  osDelay(120U);

  data[0] = 0x00U;
  ST7305_Send(ST7305_VSHLSEL, data, 1U);
  data[0] = ST7305_MADCTL_VALUE;
  ST7305_Send(ST7305_MADCTL, data, 1U);
  data[0] = 0x11U; /* XDE off; every 24-bit GRAM unit uses three writes. */
  ST7305_Send(ST7305_DTFORM, data, 1U);
  data[0] = 0x20U; /* Monochrome mode. */
  ST7305_Send(ST7305_GAMAMS, data, 1U);
  data[0] = 0x29U;
  ST7305_Send(ST7305_PNLSET, data, 1U);
  data[0] = 0x00U;
  ST7305_Send(ST7305_TEON, data, 1U);
  data[0] = 0xFFU;
  ST7305_Send(ST7305_AUTOPD, data, 1U);
  SPI_SendCommand(ST7305_HPM);
  SPI_SendCommand(ST7305_INVOFF);
  data[0] = 0x4FU;
  ST7305_Send(ST7305_CLEAR_RAM, data, 1U);
  SPI_SendCommand(ST7305_DISPON);
  osDelay(100U);

  LCD_Refresh();
  LCD_Backlight_ON;
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
  columns[1] = (uint8_t)(ST7305_COLUMN_OFFSET + x1 / 12U);
  rows[0] = (uint8_t)(ST7305_ROW_OFFSET + y0 / 2U);
  rows[1] = (uint8_t)(ST7305_ROW_OFFSET + y1 / 2U);
  ST7305_Send(ST7305_CASET, columns, sizeof(columns));
  ST7305_Send(ST7305_RASET, rows, sizeof(rows));
  SPI_SendCommand(ST7305_RAMWR);
}

void LCD_Refresh(void) {
  uint8_t line[ST7305_WIRE_ROW_BYTES];
  uint16_t y;
  uint16_t address_column;

  LCD_SetAddressWindow(0U, 0U, ST7305_WIDTH - 1U, ST7305_HEIGHT - 1U);
  for (y = 0U; y < ST7305_HEIGHT; y += 2U) {
    for (address_column = 0U; address_column < ST7305_ADDRESS_COLUMNS;
         ++address_column) {
      uint16_t x = (uint16_t)(address_column * 12U);
      size_t out = (size_t)address_column * 3U;
      line[out] = ST7305_Pack4x2(x, y);
      line[out + 1U] = ST7305_Pack4x2((uint16_t)(x + 4U), y);
      line[out + 2U] = ST7305_Pack4x2((uint16_t)(x + 8U), y);
    }
    SPI_SendBuffer(line, sizeof(line));
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
