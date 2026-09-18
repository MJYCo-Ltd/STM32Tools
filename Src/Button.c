/**
 ******************************************************************************
 * @file           : Button.c
 * @brief          : EXTI-marked button debounce and duration classification
 ******************************************************************************
 */
#include "Button.h"

#include <stddef.h>

#define BUTTON_PHASE_IDLE 0U
#define BUTTON_PHASE_PRESS_DEBOUNCE 1U
#define BUTTON_PHASE_HELD 2U
#define BUTTON_PHASE_RELEASE_DEBOUNCE 3U

static Button *volatile s_instances[BUTTON_MAX_INSTANCES];
static volatile uint16_t s_instance_pins[BUTTON_MAX_INSTANCES];
static volatile uint8_t s_instance_count;

static uint32_t Button_EnterCritical(void)
{
  const uint32_t primask = __get_PRIMASK();
  __disable_irq();
  return primask;
}

static void Button_ExitCritical(uint32_t primask)
{
  __set_PRIMASK(primask);
}

static uint8_t Button_IsActive(const Button *button)
{
  return (HAL_GPIO_ReadPin(button->port, button->pin) == button->active_state)
             ? 1U
             : 0U;
}

static GPIO_PinState Button_InactiveState(GPIO_PinState active_state)
{
  return (active_state == GPIO_PIN_RESET) ? GPIO_PIN_SET : GPIO_PIN_RESET;
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

static uint8_t Button_Register(Button *button)
{
  uint32_t primask;
  uint8_t i;
  uint8_t registered = 0U;

  primask = Button_EnterCritical();
  for (i = 0U; i < s_instance_count; ++i) {
    if (s_instances[i] == button) {
      s_instance_pins[i] = button->pin;
      registered = 1U;
      break;
    }
    if (s_instance_pins[i] == button->pin) {
      /* HAL_GPIO_EXTI_Callback only provides a pin mask, so duplicate pins
       * cannot be dispatched safely even when they belong to different ports. */
      break;
    }
  }
  if ((registered == 0U) && (i == s_instance_count) &&
      (s_instance_count < BUTTON_MAX_INSTANCES)) {
    s_instances[s_instance_count] = button;
    s_instance_pins[s_instance_count] = button->pin;
    ++s_instance_count;
    registered = 1U;
  }
  button->registered = registered;
  Button_ExitCritical(primask);
  return registered;
}

static uint8_t Button_TakeIrq(Button *button, uint32_t *irq_tick)
{
  uint32_t primask;
  uint8_t pending;

  primask = Button_EnterCritical();
  pending = button->irq_pending;
  if (pending != 0U) {
    *irq_tick = button->irq_tick;
    button->irq_pending = 0U;
  }
  Button_ExitCritical(primask);
  return pending;
}

void Button_Init(Button *button, GPIO_TypeDef *port, uint16_t pin,
                 GPIO_PinState active_state, uint32_t debounce_ms)
{
  GPIO_PinState initial_state;

  if ((button == NULL) || (port == NULL)) {
    return;
  }

  /* Reinitialization must not leave an older pin-dispatch entry behind.
   * Button_Deinit() only compares the object address before clearing fields,
   * so it is also safe for a first initialization of an unregistered object. */
  Button_Deinit(button);
  initial_state = HAL_GPIO_ReadPin(port, pin);
  button->port = port;
  button->pin = pin;
  button->active_state = active_state;
  button->debounce_ms = debounce_ms;
  button->irq_pending = 0U;
  button->irq_tick = 0U;
  button->phase = BUTTON_PHASE_IDLE;
  button->extra_long_emitted = 0U;
  button->suppress_release_event = 0U;
  button->registered = 0U;
  button->press_tick = 0U;
  button->debounce_tick = 0U;
  button->stable_state = initial_state;
  button->callback = NULL;
  button->callback_ctx = NULL;

  if (initial_state == active_state) {
    /* The physical press started before initialization, so its duration is
     * unknown. Track release to keep state correct, but suppress its event. */
    button->phase = BUTTON_PHASE_HELD;
    button->press_tick = HAL_GetTick();
    button->suppress_release_event = 1U;
  }
  (void)Button_Register(button);
}

void Button_Deinit(Button *button)
{
  uint32_t primask;
  uint8_t i;

  if (button == NULL) {
    return;
  }

  primask = Button_EnterCritical();
  for (i = 0U; i < s_instance_count; ++i) {
    if (s_instances[i] == button) {
      uint8_t j;
      for (j = i; (uint8_t)(j + 1U) < s_instance_count; ++j) {
        s_instances[j] = s_instances[j + 1U];
        s_instance_pins[j] = s_instance_pins[j + 1U];
      }
      --s_instance_count;
      s_instances[s_instance_count] = NULL;
      s_instance_pins[s_instance_count] = 0U;
      break;
    }
  }
  button->registered = 0U;
  button->irq_pending = 0U;
  button->port = NULL;
  button->callback = NULL;
  button->callback_ctx = NULL;
  Button_ExitCritical(primask);
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
  const uint8_t count = s_instance_count;

  for (i = 0U; i < count; ++i) {
    if ((s_instances[i] != NULL) && (s_instance_pins[i] == pin)) {
      Button_NotifyExti(s_instances[i]);
      return;
    }
  }
}

ButtonEvent Button_Process(Button *button)
{
  uint32_t now;
  uint32_t irq_tick = 0U;
  uint8_t pending;
  uint32_t duration_ms;
  ButtonEvent event;

  if ((button == NULL) || (button->port == NULL)) {
    return BUTTON_EVENT_NONE;
  }

  /* Take the pending flag and its timestamp as one ISR-shared snapshot. Read
   * the current time afterwards so a new EXTI cannot produce irq_tick > now. */
  pending = Button_TakeIrq(button, &irq_tick);
  now = HAL_GetTick();

  for (;;) {
    switch (button->phase) {
    case BUTTON_PHASE_IDLE:
      if (pending == 0U) {
        return BUTTON_EVENT_NONE;
      }
      button->debounce_tick = irq_tick;
      button->phase = BUTTON_PHASE_PRESS_DEBOUNCE;
      break;
    case BUTTON_PHASE_PRESS_DEBOUNCE:
      if (pending != 0U) {
        button->debounce_tick = irq_tick;
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
      if ((button->suppress_release_event == 0U) &&
          (button->extra_long_emitted == 0U) &&
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
      button->stable_state = Button_InactiveState(button->active_state);
      if ((button->suppress_release_event != 0U) ||
          (button->extra_long_emitted != 0U)) {
        button->suppress_release_event = 0U;
        button->extra_long_emitted = 0U;
        return BUTTON_EVENT_NONE;
      }
      duration_ms = button->debounce_tick - button->press_tick;
      event = Button_ClassifyDuration(duration_ms);
      return Button_Emit(button, event);
    default:
      button->phase = BUTTON_PHASE_IDLE;
      button->stable_state = Button_InactiveState(button->active_state);
      button->suppress_release_event = 0U;
      button->extra_long_emitted = 0U;
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

uint8_t Button_IsRegistered(const Button *button)
{
  return ((button != NULL) && (button->registered != 0U)) ? 1U : 0U;
}
