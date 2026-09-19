/* Unevaluated _Generic checks validate public signatures without substituting
 * implementations. Driver behavior is covered by the separate driver suites. */
#include <AHT20/aht20.h>
#include <TMP/tmp117.h>
#include <Button.h>
#include <Display/LCD/lcd_st7305.h>
#include <assert.h>
#include <stdio.h>

#define SIGNATURE(function, type) _Static_assert(_Generic(&(function), type: 1, default: 0), #function " signature changed")
typedef AHT20_Status (*AhtRead)(AHT20_Device *, AHT20_Data *);
typedef TMP117_Status (*TmpRead)(const TMP117_Device *, TMP117_Temperature *);
typedef uint32_t (*ButtonDeadline)(const Button *, uint32_t);
typedef ST7305_Status (*DisplayOperation)(void);
SIGNATURE(AHT20_Read, AhtRead);
SIGNATURE(TMP117_GetTemperature, TmpRead);
SIGNATURE(Button_NextWakeDelay, ButtonDeadline);
SIGNATURE(LCD_ST7305_Initialize, DisplayOperation);
SIGNATURE(LCD_ST7305_Reset, DisplayOperation);
SIGNATURE(LCD_ST7305_Refresh, DisplayOperation);

int main(void)
{
  assert(AHT20_I2C_ADDR7 == 0x38U);
  assert(BUTTON_WAIT_FOREVER == UINT32_MAX);
  assert(BUTTON_LONG_MS < BUTTON_EXTRA_LONG_MS);
  puts("Public API signature contracts passed (no hardware executed)");
  return 0;
}
