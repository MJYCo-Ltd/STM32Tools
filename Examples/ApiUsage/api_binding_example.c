/* Compile-checked usage, not a standalone firmware. The product owns GPIO setup,
 * RTOS waiting and the lifetime of all injected resources. No board pins here. */
#include <AHT20/aht20.h>
#include <TMP/tmp117.h>
#include <Button.h>
#include <Display/LCD/lcd_st7305.h>

AHT20_Status Example_AhtStart(AHT20_Device *sensor, const I2C_Bus *bus,
                             AHT20_DelayMsFn delay_ms, void *context)
{
  AHT20_Status status = AHT20_DeviceInit(sensor, bus, AHT20_I2C_ADDR7,
                                        delay_ms, context);
  return status == AHT20_OK ? AHT20_Initialize(sensor) : status;
}

TMP117_Status Example_TmpRead(TMP117_Device *sensor, const I2C_Bus *bus,
                             TMP117_Temperature *temperature)
{
  TMP117_Status status = TMP117_DeviceInit(sensor, bus, TMP117_ADDR_GND);
  return status == TMP117_OK ? TMP117_GetTemperature(sensor, temperature) : status;
}

uint8_t Example_ButtonStart(Button *button, GPIO_TypeDef *port, uint16_t pin)
{
  /* Board MUST configure GPIO_MODE_IT_RISING_FALLING before this call. */
  Button_Init(button, port, pin, GPIO_PIN_RESET, 30U);
  return Button_IsRegistered(button);
}

uint32_t Example_ButtonPump(Button *button, ButtonEvent *event)
{
  /* The owning task waits for an EXTI notification or this deadline, whichever
   * comes first. Convert milliseconds to RTOS ticks; preserve FOREVER and 0.
   * Call Button_Deinit before the button object goes out of scope. */
  *event = Button_Process(button);
  return Button_NextWakeDelay(button, HAL_GetTick());
}

ST7305_Status Example_DisplayStart(const ST7305_Binding *binding)
{
  ST7305_Status status = LCD_ST7305_Bind(binding);
  return status == ST7305_OK ? LCD_ST7305_Initialize() : status;
}
