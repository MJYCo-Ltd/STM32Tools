# Display（SPI Bus / Graphics / LCD / EPD）

## Boundary

STM32Tools owns reusable display protocol and rendering code:

- `SPI_DisplayBus` transport abstraction;
- generic drawing primitives;
- LCD/EPD controller protocols;
- reusable panel profiles that contain controller timing and geometry only.

The product/board repository owns:

- the concrete SPI peripheral;
- CS/DC/RESET/backlight/TE GPIO bindings;
- RTOS or HAL delay policy;
- framebuffer and line-buffer memory allocation;
- which physical panel is fitted and how it is mounted/rotated.

Reusable drivers must not include a product `main.h` or select `hspi1` and GPIO
macros internally.

## SPI display bus

`Inc/Display/spi_display_bus.h` defines the platform-neutral bus contract.
`Src/Display/stm32_spi_display_bus.c` is the STM32 HAL port; the caller supplies
the SPI handle and board GPIOs.

```c
SPI_DisplayBus bus;
SPI_DisplayBusInitSTM32(&bus, &hspi1,
                        LCD_CS_GPIO_Port, LCD_CS_Pin,
                        LCD_DC_GPIO_Port, LCD_DC_Pin,
                        0U);
```

## ST7305 explicit binding

`lcd_st7305.c` contains the reusable ST7305 controller, framebuffer packing,
rotation and refresh logic. It no longer owns board pins, an RTOS delay, or
framebuffer storage.

The supplied profile
`ST7305_PANEL_FD042MN_ZF21_H06_B` describes the 300x400 FOCUS DISPLAY panel.
The product binds it to board resources:

```c
static SPI_DisplayBus bus;
static uint8_t framebuffer[
    ST7305_FD042MN_ZF21_H06_B_FRAMEBUFFER_SIZE];
static uint8_t line_buffer[
    ST7305_FD042MN_ZF21_H06_B_LINE_BUFFER_SIZE];

static void Reset(void *context, uint8_t asserted)
{
    (void)context;
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin,
        asserted ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void Delay(void *context, uint32_t ms)
{
    (void)context;
    osDelay(ms);
}

SPI_DisplayBusInitSTM32(&bus, &hspi1,
                        LCD_CS_GPIO_Port, LCD_CS_Pin,
                        LCD_DC_GPIO_Port, LCD_DC_Pin, 0U);

const ST7305_Binding binding = {
    .bus = &bus,
    .panel = &ST7305_PANEL_FD042MN_ZF21_H06_B,
    .reset = Reset,
    .delay_ms = Delay,
    .framebuffer = framebuffer,
    .framebuffer_size = sizeof(framebuffer),
    .line_buffer = line_buffer,
    .line_buffer_size = sizeof(line_buffer),
    .rotation = ROTATION_270,
};

LCD_ST7305_Bind(&binding);
LCD_ST7305_Initialize();
```

The existing `LCD_*` and `DrawPixel()` facade remains available after binding,
so `Graphics.c` and existing UI code do not need to know the board wiring.

## Graphics API

`Graphics.c` builds lines, rectangles, circles, triangles and the basic 5x7 font
on top of the selected backend's `DrawPixel()` implementation.

Important: the third argument of `DrawHLine()`/`DrawVLine()` is the end
coordinate, not a length.

## Other backends

ST7789 and existing EPD drivers still use their legacy configuration headers and
macros. They remain functional, but should eventually adopt the same explicit
transport/profile/buffer binding pattern used by ST7305.

## Tests

`Test/st7305_driver_test.c` uses a fake SPI bus and verifies:

- binding and buffer-size validation;
- hardware-reset and software-reset paths;
- panel initialization commands and delays;
- pixel packing and partial refresh;
- absence of STM32 HAL, `main.h`, and RTOS dependencies in the controller.
