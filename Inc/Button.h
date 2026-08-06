/**
 ******************************************************************************
 * @file           : Button.h
 * @brief          : Non-blocking GPIO button debounce helper
 ******************************************************************************
 */
#ifndef STM32TOOLS_BUTTON_H
#define STM32TOOLS_BUTTON_H

#include "main.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BUTTON_EVENT_NONE = 0,
  BUTTON_EVENT_PRESSED,
  BUTTON_EVENT_RELEASED
} ButtonEvent;

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState active_state;
  GPIO_PinState raw_state;
  GPIO_PinState stable_state;
  uint32_t raw_changed_tick;
  uint32_t debounce_ms;
} Button;

/** Initialize a button. Call after its GPIO has been configured. */
void Button_Init(Button *button, GPIO_TypeDef *port, uint16_t pin,
                 GPIO_PinState active_state, uint32_t debounce_ms);

/** Poll frequently; returns one event after a state is stable for debounce_ms. */
ButtonEvent Button_Poll(Button *button);

/** Return the last debounced logical state without reading the GPIO again. */
uint8_t Button_IsPressed(const Button *button);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_BUTTON_H */
