/**
 ******************************************************************************
 * @file           : Button.c
 * @brief          : EXTI-marked button debounce and duration classification
 ******************************************************************************
 */
#include "Button.h"

#include <stddef.h>

#define BUTTON_MAX_INSTANCES 4U

#define BUTTON_PHASE_IDLE 0U
#define BUTTON_PHASE_PRESS_DEBOUNCE 1U
#define BUTTON_PHASE_HELD 2U
#define BUTTON_PHASE_RELEASE_DEBOUNCE 3U

static Button *s_instances[BUTTON_MAX_INSTANCES];
static uint8_t s_instance_count;

static uint8_t Button_IsActive(const Button *button)
{
  return (HAL_GPIO_ReadPin(button->port, button->pin) == button->active_state)
             ? 1U
             : 0U;
}

static ButtonEvent Button_Emit(Button *button, ButtonEvent event)
{
  if (button->callback != NULL) {
    button->callback(button->callback_ctx, event);
  }
  return event;
}

static ButtonEvent Button_ClassifyDuration(uint32_t duration_ms)
{
  if (duration_ms >= BUTTON_EXTRA_LONG_MS) {
    return BUTTON_EVENT_EXTRA_LONG;
  }
  if (duration_ms >= BUTTON_LONG_MS) {
    return BUTTON_EVENT_LONG;
  }
  return BUTTON_EVENT_SHORT;
}

static void Button_Register(Button *button)
{
  uint8_t i;

  for (i = 0U; i < s_instance_count; ++i) {
    if ((s_instances[i] == button) || (s_instances[i]->pin == button->pin)) {
      s_instances[i] = button;
      return;
    }
  }
  if (s_instance_count < BUTTON_MAX_INSTANCES) {
    s_instances[s_instance_count++] = button;
  }
}

void Button_Init(Button *button, GPIO_TypeDef *port, uint16_t pin,
                 GPIO_PinState active_state, uint32_t debounce_ms)
{
  if ((button == NULL) || (port == NULL)) {
    return;
  }

  button->port = port;
  button->pin = pin;
  button->active_state = active_state;
  button->debounce_ms = debounce_ms;
  button->irq_pending = 0U;
  button->irq_tick = 0U;
  button->phase = BUTTON_PHASE_IDLE;
  button->extra_long_emitted = 0U;
  button->press_tick = 0U;
  button->debounce_tick = 0U;
  button->stable_state = HAL_GPIO_ReadPin(port, pin);
  button->callback = NULL;
  button->callback_ctx = NULL;
  Button_Register(button);
}

void Button_SetCallback(Button *button, ButtonEventCallback callback, void *ctx)
{
  if (button == NULL) {
    return;
  }
  button->callback = callback;
  button->callback_ctx = ctx;
}

void Button_NotifyExti(Button *button)
{
  if ((button == NULL) || (button->port == NULL)) {
    return;
  }
  button->irq_tick = HAL_GetTick();
  button->irq_pending = 1U;
}

void Button_NotifyExtiPin(uint16_t pin)
{
  uint8_t i;

  for (i = 0U; i < s_instance_count; ++i) {
    if ((s_instances[i] != NULL) && (s_instances[i]->pin == pin)) {
      Button_NotifyExti(s_instances[i]);
      return;
    }
  }
}

ButtonEvent Button_Process(Button *button)
{
  uint32_t now;
  uint8_t pending;
  uint32_t duration_ms;
  ButtonEvent event;

  if ((button == NULL) || (button->port == NULL)) {
    return BUTTON_EVENT_NONE;
  }

  now = HAL_GetTick();
  pending = button->irq_pending;
  if (pending != 0U) {
    button->irq_pending = 0U;
  }

  for (;;) {
    switch (button->phase) {
    case BUTTON_PHASE_IDLE:
      if (pending == 0U) {
        return BUTTON_EVENT_NONE;
      }
      button->debounce_tick = button->irq_tick;
      button->phase = BUTTON_PHASE_PRESS_DEBOUNCE;
      break;
    case BUTTON_PHASE_PRESS_DEBOUNCE:
      if (pending != 0U) {
        button->debounce_tick = button->irq_tick;
        pending = 0U;
      }
      if (Button_IsActive(button) == 0U) {
        button->phase = BUTTON_PHASE_IDLE;
        return BUTTON_EVENT_NONE;
      }
      if ((now - button->debounce_tick) < button->debounce_ms) {
        return BUTTON_EVENT_NONE;
      }
      button->phase = BUTTON_PHASE_HELD;
      button->press_tick = button->debounce_tick;
      button->extra_long_emitted = 0U;
      button->stable_state = button->active_state;
      break;
    case BUTTON_PHASE_HELD:
      if (Button_IsActive(button) == 0U) {
        button->phase = BUTTON_PHASE_RELEASE_DEBOUNCE;
        button->debounce_tick = now;
        return BUTTON_EVENT_NONE;
      }
      if ((button->extra_long_emitted == 0U) &&
          ((now - button->press_tick) >= BUTTON_EXTRA_LONG_MS)) {
        button->extra_long_emitted = 1U;
        return Button_Emit(button, BUTTON_EVENT_EXTRA_LONG);
      }
      return BUTTON_EVENT_NONE;
    case BUTTON_PHASE_RELEASE_DEBOUNCE:
      if (Button_IsActive(button) != 0U) {
        button->phase = BUTTON_PHASE_HELD;
        return BUTTON_EVENT_NONE;
      }
      if ((now - button->debounce_tick) < button->debounce_ms) {
        return BUTTON_EVENT_NONE;
      }
      button->phase = BUTTON_PHASE_IDLE;
      button->stable_state = (button->active_state == GPIO_PIN_RESET)
                                 ? GPIO_PIN_SET
                                 : GPIO_PIN_RESET;
      if (button->extra_long_emitted != 0U) {
        button->extra_long_emitted = 0U;
        return BUTTON_EVENT_NONE;
      }
      duration_ms = button->debounce_tick - button->press_tick;
      event = Button_ClassifyDuration(duration_ms);
      return Button_Emit(button, event);
    default:
      button->phase = BUTTON_PHASE_IDLE;
      return BUTTON_EVENT_NONE;
    }
  }
}

uint8_t Button_IsPressed(const Button *button)
{
  if ((button == NULL) || (button->port == NULL)) {
    return 0U;
  }
  return (button->stable_state == button->active_state) ? 1U : 0U;
}
