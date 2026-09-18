/**
 ******************************************************************************
 * @file           : Button.c
 * @brief          : Dual-edge EXTI button debounce and duration classification
 ******************************************************************************
 */
#include "Button.h"

#include <stddef.h>

#define BUTTON_PHASE_IDLE     0U
#define BUTTON_PHASE_DEBOUNCE 1U
#define BUTTON_PHASE_HELD     2U

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

static GPIO_PinState Button_StateFromActive(const Button *button,
                                            uint8_t active)
{
  if (active != 0U) {
    return button->active_state;
  }
  return (button->active_state == GPIO_PIN_RESET) ? GPIO_PIN_SET
                                                   : GPIO_PIN_RESET;
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

static uint32_t Button_Remaining(uint32_t now_ms, uint32_t started_ms,
                                 uint32_t duration_ms)
{
  const uint32_t elapsed = now_ms - started_ms;
  return (elapsed >= duration_ms) ? 0U : (duration_ms - elapsed);
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

static uint8_t Button_TakeIrq(Button *button, uint32_t *irq_tick,
                              uint8_t *irq_active)
{
  uint32_t primask;
  uint8_t pending;

  primask = Button_EnterCritical();
  pending = button->irq_pending;
  if (pending != 0U) {
    *irq_tick = button->irq_tick;
    *irq_active = button->irq_active;
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

  /* Reinitialization must not leave an older pin-dispatch entry behind. */
  Button_Deinit(button);
  initial_state = HAL_GPIO_ReadPin(port, pin);
  button->port = port;
  button->pin = pin;
  button->active_state = active_state;
  button->debounce_ms = debounce_ms;
  button->irq_pending = 0U;
  button->irq_active = 0U;
  button->irq_tick = 0U;
  button->phase = BUTTON_PHASE_IDLE;
  button->debounce_active = 0U;
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
     * unknown. Track the rising edge, but suppress an event for this hold. */
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
  button->irq_active = Button_IsActive(button);
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
  uint8_t irq_active = 0U;
  uint8_t pending;
  uint8_t active_now;
  uint8_t stable_active;
  uint32_t duration_ms;
  ButtonEvent event;

  if ((button == NULL) || (button->port == NULL)) {
    return BUTTON_EVENT_NONE;
  }

  /* Snapshot all ISR-shared edge data before reading the current time. */
  pending = Button_TakeIrq(button, &irq_tick, &irq_active);
  now = HAL_GetTick();
  if (pending != 0U) {
    button->debounce_tick = irq_tick;
    button->debounce_active = irq_active;
    button->phase = BUTTON_PHASE_DEBOUNCE;
  }

  if (button->phase == BUTTON_PHASE_DEBOUNCE) {
    if (Button_Remaining(now, button->debounce_tick, button->debounce_ms) !=
        0U) {
      return BUTTON_EVENT_NONE;
    }

    /* Read once at the one-shot debounce deadline. Stable idle/held states do
     * not poll GPIO. If an edge was missed or is still bouncing, restart the
     * one-shot window from the observed level. */
    active_now = Button_IsActive(button);
    if (active_now != button->debounce_active) {
      button->debounce_active = active_now;
      button->debounce_tick = now;
      return BUTTON_EVENT_NONE;
    }

    stable_active = (button->stable_state == button->active_state) ? 1U : 0U;
    if (active_now == stable_active) {
      button->phase = (active_now != 0U) ? BUTTON_PHASE_HELD
                                         : BUTTON_PHASE_IDLE;
      return BUTTON_EVENT_NONE;
    }

    button->stable_state = Button_StateFromActive(button, active_now);
    if (active_now != 0U) {
      button->phase = BUTTON_PHASE_HELD;
      button->press_tick = button->debounce_tick;
      button->extra_long_emitted = 0U;
      button->suppress_release_event = 0U;
      return BUTTON_EVENT_NONE;
    }

    button->phase = BUTTON_PHASE_IDLE;
    if ((button->suppress_release_event != 0U) ||
        (button->extra_long_emitted != 0U)) {
      button->suppress_release_event = 0U;
      button->extra_long_emitted = 0U;
      return BUTTON_EVENT_NONE;
    }
    duration_ms = button->debounce_tick - button->press_tick;
    event = Button_ClassifyDuration(duration_ms);
    return Button_Emit(button, event);
  }

  if (button->phase == BUTTON_PHASE_HELD) {
    if ((button->suppress_release_event == 0U) &&
        (button->extra_long_emitted == 0U) &&
        (Button_Remaining(now, button->press_tick, BUTTON_EXTRA_LONG_MS) ==
         0U)) {
      button->extra_long_emitted = 1U;
      return Button_Emit(button, BUTTON_EVENT_EXTRA_LONG);
    }
    return BUTTON_EVENT_NONE;
  }

  button->phase = BUTTON_PHASE_IDLE;
  return BUTTON_EVENT_NONE;
}

uint32_t Button_NextWakeDelay(const Button *button, uint32_t now_ms)
{
  if ((button == NULL) || (button->port == NULL)) {
    return BUTTON_WAIT_FOREVER;
  }
  if (button->irq_pending != 0U) {
    return 0U;
  }
  if (button->phase == BUTTON_PHASE_DEBOUNCE) {
    return Button_Remaining(now_ms, button->debounce_tick,
                            button->debounce_ms);
  }
  if ((button->phase == BUTTON_PHASE_HELD) &&
      (button->suppress_release_event == 0U) &&
      (button->extra_long_emitted == 0U)) {
    return Button_Remaining(now_ms, button->press_tick,
                            BUTTON_EXTRA_LONG_MS);
  }
  return BUTTON_WAIT_FOREVER;
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
