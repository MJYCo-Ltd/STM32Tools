#include <Display/LCD/lcd_st7305.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  unsigned calls, init_commands, resets, fail_at;
  uint8_t mode, require_not_ready;
} Fake;
static void Mode(const SPI_DisplayBus *bus, uint8_t mode)
{ ((Fake *)bus->spi_handle)->mode = mode; }
static int Transfer(const SPI_DisplayBus *bus, const uint8_t *data,
                    size_t length, uint8_t dma)
{
  Fake *fake = bus->spi_handle;
  (void)dma;
  assert(data != NULL && length != 0U);
  if (fake->require_not_ready) assert(!LCD_ST7305_IsReady());
  ++fake->calls;
  if (!fake->mode && length == 1U && *data == 0x29U) ++fake->init_commands;
  return fake->fail_at == fake->calls ? -1 : 0;
}
static void Reset(void *context, uint8_t asserted)
{
  Fake *fake = context;
  assert(!LCD_ST7305_IsReady());
  if (asserted) ++fake->resets;
}
static void Delay(void *context, uint32_t ms)
{ (void)context; (void)ms; }
static const SPI_DisplayCommand sequence[] = {{0x29U, NULL, 0U, 0U}};
static const ST7305_PanelProfile panel = {
  .width = 12U, .height = 4U, .column_offset = 1U, .column_end = 1U,
  .row_offset = 0U, .init_sequence = sequence, .init_sequence_count = 1U
};

static void CheckFailureAt(unsigned fail_at, uint8_t hardware_reset)
{
  Fake fake = {.fail_at = fail_at, .require_not_ready = 1U};
  SPI_DisplayBus bus = {.spi_handle = &fake, .transfer = Transfer, .data_mode = Mode};
  uint8_t framebuffer[8] = {0}, line[3] = {0};
  ST7305_Binding binding = {
    .bus = &bus, .panel = &panel, .reset = hardware_reset ? Reset : NULL,
    .delay_ms = Delay, .io_context = &fake, .framebuffer = framebuffer,
    .framebuffer_size = sizeof(framebuffer), .line_buffer = line,
    .line_buffer_size = sizeof(line), .rotation = NO_ROTATION
  };
  assert(LCD_ST7305_Bind(&binding) == ST7305_OK);
  assert(!LCD_ST7305_IsReady());
  assert(LCD_ST7305_Initialize() == ST7305_ERR_IO);
  assert(fake.calls == fail_at); /* Stop at the first transport error. */
  assert(!LCD_ST7305_IsReady());
  assert(LCD_ST7305_Refresh() == ST7305_ERR_STATE);
  fake.fail_at = 0U;
  unsigned before = fake.calls;
  assert(LCD_ST7305_Initialize() == ST7305_OK);
  assert(fake.calls > before && LCD_ST7305_IsReady());

  fake.require_not_ready = 0U;
  before = fake.calls;
  framebuffer[0] = 0x80U;
  assert(LCD_ST7305_Initialize() == ST7305_OK);
  assert(fake.calls == before && framebuffer[0] == 0x80U);

  LCD_Reset(); /* Legacy reset must invalidate the same ready state. */
  assert(!LCD_ST7305_IsReady());
  fake.require_not_ready = 1U;
  before = fake.init_commands;
  assert(LCD_ST7305_Initialize() == ST7305_OK);
  assert(fake.init_commands > before && LCD_ST7305_IsReady());

  fake.require_not_ready = 0U;
  assert(LCD_ST7305_RefreshArea(0U, 0U, 0U, 1U) == ST7305_ERR_PARAM);
  assert(LCD_ST7305_IsReady()); /* Bad geometry is not a controller failure. */
  fake.fail_at = fake.calls + 1U;
  assert(LCD_ST7305_Refresh() == ST7305_ERR_IO);
  assert(!LCD_ST7305_IsReady());
  fake.fail_at = 0U;
  assert(LCD_ST7305_Initialize() == ST7305_OK);
  fake.fail_at = fake.calls + 1U;
  LCD_Refresh(); /* The void compatibility facade must also invalidate ready. */
  assert(!LCD_ST7305_IsReady());
  LCD_ST7305_Unbind();
  assert(!LCD_ST7305_IsReady() && !LCD_ST7305_IsBound());
  assert(LCD_ST7305_Initialize() == ST7305_ERR_STATE);
  assert(LCD_ST7305_Reset() == ST7305_ERR_STATE);
}

int main(void)
{
  /* One init command, then 2 rows x (CASET cmd/data, RASET cmd/data,
   * RAMWR cmd/data): cover every transfer, not just the first refresh call. */
  for (unsigned i = 1U; i <= 13U; ++i) CheckFailureAt(i, 1U);
  for (unsigned i = 1U; i <= 14U; ++i) CheckFailureAt(i, 0U);
  puts("st7305_lifecycle_test: 27 fault positions and recovery contracts passed");
  return 0;
}
