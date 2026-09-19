#include "Display/Graphics.h"
#include "Display/LCD/lcd_st7305.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LOG_CAPACITY 4096U
#define LOG_BYTES 16U
#define DELAY_CAPACITY 64U

typedef struct {
  uint8_t data_mode;
  size_t length;
  uint8_t bytes[LOG_BYTES];
} TransferLog;

typedef struct {
  TransferLog logs[LOG_CAPACITY];
  size_t log_count;
  uint8_t current_mode;
  uint8_t selected;
  uint8_t reset_states[8];
  size_t reset_count;
  uint32_t delays[DELAY_CAPACITY];
  size_t delay_count;
} FakeDisplay;

static FakeDisplay *FakeFromBus(const SPI_DisplayBus *bus)
{
  return (FakeDisplay *)bus->spi_handle;
}

static void FakeSelect(const SPI_DisplayBus *bus, uint8_t active)
{
  FakeDisplay *fake = FakeFromBus(bus);
  fake->selected = active;
}

static void FakeDataMode(const SPI_DisplayBus *bus, uint8_t data_mode)
{
  FakeDisplay *fake = FakeFromBus(bus);
  fake->current_mode = data_mode;
}

static int FakeTransfer(const SPI_DisplayBus *bus, const uint8_t *data,
                        size_t length, uint8_t use_dma)
{
  FakeDisplay *fake = FakeFromBus(bus);
  TransferLog *log;
  size_t copy;

  (void)use_dma;
  assert(fake != NULL);
  assert(fake->selected != 0U);
  assert(data != NULL && length != 0U);
  assert(fake->log_count < LOG_CAPACITY);

  log = &fake->logs[fake->log_count++];
  memset(log, 0, sizeof(*log));
  log->data_mode = fake->current_mode;
  log->length = length;
  copy = (length < sizeof(log->bytes)) ? length : sizeof(log->bytes);
  memcpy(log->bytes, data, copy);
  return 0;
}

static void FakeReset(void *context, uint8_t asserted)
{
  FakeDisplay *fake = context;
  assert(fake->reset_count < sizeof(fake->reset_states));
  fake->reset_states[fake->reset_count++] = asserted;
}

static void FakeDelay(void *context, uint32_t delay_ms)
{
  FakeDisplay *fake = context;
  assert(fake->delay_count < DELAY_CAPACITY);
  fake->delays[fake->delay_count++] = delay_ms;
}

static SPI_DisplayBus MakeBus(FakeDisplay *fake)
{
  SPI_DisplayBus bus;

  memset(&bus, 0, sizeof(bus));
  bus.spi_handle = fake;
  bus.dma_threshold = 16U;
  bus.use_dma = 0U;
  bus.transfer = FakeTransfer;
  bus.select = FakeSelect;
  bus.data_mode = FakeDataMode;
  return bus;
}

static void ResetLogs(FakeDisplay *fake)
{
  fake->log_count = 0U;
  fake->selected = 0U;
  fake->current_mode = 0U;
}

static size_t FindCommand(const FakeDisplay *fake, uint8_t command)
{
  size_t index;
  for (index = 0U; index < fake->log_count; ++index) {
    if ((fake->logs[index].data_mode == 0U) &&
        (fake->logs[index].length == 1U) &&
        (fake->logs[index].bytes[0] == command)) {
      return index;
    }
  }
  return SIZE_MAX;
}

static void test_sizes_and_binding_validation(void)
{
  FakeDisplay fake = {0};
  SPI_DisplayBus bus = MakeBus(&fake);
  static uint8_t framebuffer[ST7305_FD042MN_ZF21_H06_B_FRAMEBUFFER_SIZE];
  static uint8_t line[ST7305_FD042MN_ZF21_H06_B_LINE_BUFFER_SIZE];
  ST7305_Binding binding = {
      .bus = &bus,
      .panel = &ST7305_PANEL_FD042MN_ZF21_H06_B,
      .reset = FakeReset,
      .delay_ms = FakeDelay,
      .io_context = &fake,
      .framebuffer = framebuffer,
      .framebuffer_size = sizeof(framebuffer),
      .line_buffer = line,
      .line_buffer_size = sizeof(line),
      .rotation = NO_ROTATION};

  assert(ST7305_FramebufferSize(&ST7305_PANEL_FD042MN_ZF21_H06_B) ==
         ST7305_FD042MN_ZF21_H06_B_FRAMEBUFFER_SIZE);
  assert(ST7305_LineBufferSize(&ST7305_PANEL_FD042MN_ZF21_H06_B) ==
         ST7305_FD042MN_ZF21_H06_B_LINE_BUFFER_SIZE);
  assert(LCD_ST7305_Bind(NULL) == ST7305_ERR_PARAM);

  binding.framebuffer_size = sizeof(framebuffer) - 1U;
  assert(LCD_ST7305_Bind(&binding) == ST7305_ERR_PARAM);
  binding.framebuffer_size = sizeof(framebuffer);
  binding.line_buffer_size = sizeof(line) - 1U;
  assert(LCD_ST7305_Bind(&binding) == ST7305_ERR_PARAM);
  binding.line_buffer_size = sizeof(line);
  binding.delay_ms = NULL;
  assert(LCD_ST7305_Bind(&binding) == ST7305_ERR_PARAM);

  binding.delay_ms = FakeDelay;
  assert(LCD_ST7305_Bind(&binding) == ST7305_OK);
  assert(LCD_ST7305_IsBound() != 0U);
  LCD_ST7305_Unbind();
  assert(LCD_ST7305_IsBound() == 0U);
}

static void test_init_sequence_and_hardware_reset(void)
{
  FakeDisplay fake = {0};
  SPI_DisplayBus bus = MakeBus(&fake);
  static uint8_t framebuffer[ST7305_FD042MN_ZF21_H06_B_FRAMEBUFFER_SIZE];
  static uint8_t line[ST7305_FD042MN_ZF21_H06_B_LINE_BUFFER_SIZE];
  ST7305_Binding binding = {
      .bus = &bus,
      .panel = &ST7305_PANEL_FD042MN_ZF21_H06_B,
      .reset = FakeReset,
      .delay_ms = FakeDelay,
      .io_context = &fake,
      .framebuffer = framebuffer,
      .framebuffer_size = sizeof(framebuffer),
      .line_buffer = line,
      .line_buffer_size = sizeof(line),
      .rotation = ROTATION_270};
  size_t nvm;
  size_t display_on;

  memset(framebuffer, 0xFF, sizeof(framebuffer));
  assert(LCD_ST7305_Bind(&binding) == ST7305_OK);
  assert(LCD_ST7305_Initialize() == ST7305_OK);
  assert(fake.reset_count == 2U);
  assert(fake.reset_states[0] == 1U && fake.reset_states[1] == 0U);
  assert(fake.delay_count >= 4U);
  assert(fake.delays[0] == 10U && fake.delays[1] == 100U);
  assert(memcmp(framebuffer, (uint8_t[16]){0}, 16U) == 0);

  nvm = FindCommand(&fake, 0xD6U);
  assert(nvm != SIZE_MAX && (nvm + 1U) < fake.log_count);
  assert(fake.logs[nvm + 1U].data_mode == 1U);
  assert(fake.logs[nvm + 1U].length == 2U);
  assert(fake.logs[nvm + 1U].bytes[0] == 0x13U);
  assert(fake.logs[nvm + 1U].bytes[1] == 0x02U);
  display_on = FindCommand(&fake, 0x29U);
  assert(display_on != SIZE_MAX);
  LCD_ST7305_Unbind();
}

static void test_draw_and_partial_refresh(void)
{
  FakeDisplay fake = {0};
  SPI_DisplayBus bus = MakeBus(&fake);
  static uint8_t framebuffer[ST7305_FD042MN_ZF21_H06_B_FRAMEBUFFER_SIZE];
  static uint8_t line[ST7305_FD042MN_ZF21_H06_B_LINE_BUFFER_SIZE];
  ST7305_Binding binding = {
      .bus = &bus,
      .panel = &ST7305_PANEL_FD042MN_ZF21_H06_B,
      .reset = FakeReset,
      .delay_ms = FakeDelay,
      .io_context = &fake,
      .framebuffer = framebuffer,
      .framebuffer_size = sizeof(framebuffer),
      .line_buffer = line,
      .line_buffer_size = sizeof(line),
      .rotation = NO_ROTATION};
  Pixel black = {.x = 0U, .y = 0U, .color = {0U, 0U, 0U}};
  Pixel white = {.x = 0U, .y = 0U, .color = {255U, 255U, 255U}};
  const TransferLog *wire;

  assert(LCD_ST7305_Bind(&binding) == ST7305_OK);
  assert(LCD_ST7305_Initialize() == ST7305_OK);

  ResetLogs(&fake);
  DrawPixel(&black);
  LCD_RefreshArea(0U, 0U, 1U, 1U);
  assert(fake.log_count == 6U);
  wire = &fake.logs[fake.log_count - 1U];
  assert(wire->data_mode == 1U);
  assert(wire->length == ST7305_FD042MN_ZF21_H06_B_LINE_BUFFER_SIZE);
  assert(wire->bytes[0] == 0x80U);

  ResetLogs(&fake);
  DrawPixel(&white);
  LCD_RefreshArea(0U, 0U, 1U, 1U);
  wire = &fake.logs[fake.log_count - 1U];
  assert(wire->bytes[0] == 0x00U);
  LCD_ST7305_Unbind();
}

static void test_software_reset_fallback(void)
{
  FakeDisplay fake = {0};
  SPI_DisplayBus bus = MakeBus(&fake);
  static uint8_t framebuffer[ST7305_FD042MN_ZF21_H06_B_FRAMEBUFFER_SIZE];
  static uint8_t line[ST7305_FD042MN_ZF21_H06_B_LINE_BUFFER_SIZE];
  ST7305_Binding binding = {
      .bus = &bus,
      .panel = &ST7305_PANEL_FD042MN_ZF21_H06_B,
      .reset = NULL,
      .delay_ms = FakeDelay,
      .io_context = &fake,
      .framebuffer = framebuffer,
      .framebuffer_size = sizeof(framebuffer),
      .line_buffer = line,
      .line_buffer_size = sizeof(line),
      .rotation = NO_ROTATION};

  assert(LCD_ST7305_Bind(&binding) == ST7305_OK);
  assert(LCD_ST7305_Initialize() == ST7305_OK);
  assert(fake.reset_count == 0U);
  assert(fake.log_count != 0U);
  assert(fake.logs[0].data_mode == 0U);
  assert(fake.logs[0].bytes[0] == 0x01U);
  assert(fake.delays[0] == 100U);
  LCD_ST7305_Unbind();
}

int main(void)
{
  test_sizes_and_binding_validation();
  test_init_sequence_and_hardware_reset();
  test_draw_and_partial_refresh();
  test_software_reset_fallback();
  puts("ST7305 explicit binding tests passed");
  return 0;
}
