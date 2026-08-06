/**
 ******************************************************************************
 * @file           : Button.c
 * @brief          : Non-blocking GPIO button debounce helper implementation
 ******************************************************************************
 */
#include "Button.h"

#include <stddef.h>

void Button_Init(Button *button, GPIO_TypeDef *port, uint16_t pin,
                 GPIO_PinState active_state, uint32_t debounce_ms)
{
  GPIO_PinState initial_state;

  if ((button == NULL) || (port == NULL)) {
    return;
  }

  initial_state = HAL_GPIO_ReadPin(port, pin);
  button->port = port;
  button->pin = pin;
  button->active_state = active_state;
  button->raw_state = initial_state;
  button->stable_state = initial_state;
  button->raw_changed_tick = HAL_GetTick();
  button->debounce_ms = debounce_ms;
}

ButtonEvent Button_Poll(Button *button)
{
  GPIO_PinState current_state;
  uint32_t now;

  if ((button == NULL) || (button->port == NULL)) {
    return BUTTON_EVENT_NONE;
  }

  now = HAL_GetTick();
  current_state = HAL_GPIO_ReadPin(button->port, button->pin);
  if (current_state != button->raw_state) {
    button->raw_state = current_state;
    button->raw_changed_tick = now;
  }

  if ((button->stable_state != button->raw_state) &&
      ((now - button->raw_changed_tick) >= button->debounce_ms)) {
    button->stable_state = button->raw_state;
    return (button->stable_state == button->active_state)
               ? BUTTON_EVENT_PRESSED
               : BUTTON_EVENT_RELEASED;
  }

  return BUTTON_EVENT_NONE;
}

uint8_t Button_IsPressed(const Button *button)
{
  if ((button == NULL) || (button->port == NULL)) {
    return 0U;
  }
  return (button->stable_state == button->active_state) ? 1U : 0U;
}
