/**
 ******************************************************************************
 * @file           : Button.c
 * @brief          : Dual-edge EXTI button debounce and duration classification
 ******************************************************************************
 */
#include "Button.h"
#include <stddef.h>
#include <string.h>

#define BUTTON_PHASE_IDLE 0U
#define BUTTON_PHASE_DEBOUNCE 1U
#define BUTTON_PHASE_HELD 2U
_Static_assert(BUTTON_MAX_INSTANCES > 0U && BUTTON_MAX_INSTANCES <= 16U,
               "STM32 EXTI supports at most sixteen distinct pin lines");
static Button *s_instances[BUTTON_MAX_INSTANCES];

static uint32_t Lock(void) {
  uint32_t p = __get_PRIMASK();
  __disable_irq();
  return p;
}
static void Unlock(uint32_t p) {
  __set_PRIMASK(p);
}
static uint8_t Active(const Button *b) {
  return HAL_GPIO_ReadPin(b->port, b->pin) == b->active_state;
}
static GPIO_PinState State(const Button *b, uint8_t active) {
  return active ? b->active_state
                : (b->active_state == GPIO_PIN_RESET ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
static uint32_t Remaining(uint32_t now, uint32_t start, uint32_t duration) {
  uint32_t elapsed = now - start;
  return elapsed >= duration ? 0U : duration - elapsed;
}

void Button_Deinit(Button *b) {
  if (b == NULL)
    return;
  uint32_t p = Lock();
  for (unsigned i = 0; i < BUTTON_MAX_INSTANCES; ++i)
    if (s_instances[i] == b)
      s_instances[i] = NULL;
  /* Safe for an uninitialized object: never read its previous contents. */
  memset(b, 0, sizeof(*b));
  Unlock(p);
}

void Button_Init(Button *b, GPIO_TypeDef *port, uint16_t pin, GPIO_PinState active,
                 uint32_t debounce) {
  if (b == NULL)
    return;
  Button_Deinit(b);
  if (port == NULL || pin == 0U || (pin & (uint16_t)(pin - 1U)) != 0U ||
      (active != GPIO_PIN_RESET && active != GPIO_PIN_SET) || debounce >= 0x80000000UL)
    return;
  uint32_t p = Lock();
  b->port = port;
  b->pin = pin;
  b->active_state = active;
  b->debounce_ms = debounce;
  b->stable_state = HAL_GPIO_ReadPin(port, pin);
  if (b->stable_state == active) {
    b->phase = BUTTON_PHASE_HELD;
    b->suppress_release_event = 1U;
  }
  unsigned free_slot = BUTTON_MAX_INSTANCES;
  for (unsigned i = 0; i < BUTTON_MAX_INSTANCES; ++i) {
    if (s_instances[i] != NULL && s_instances[i]->pin == pin) {
      Unlock(p);
      return;
    }
    if (s_instances[i] == NULL && free_slot == BUTTON_MAX_INSTANCES)
      free_slot = i;
  }
  if (free_slot < BUTTON_MAX_INSTANCES) {
    s_instances[free_slot] = b;
    b->registered = 1U;
  }
  Unlock(p);
}

void Button_SetCallback(Button *b, ButtonEventCallback cb, void *ctx) {
  if (b == NULL)
    return;
  b->callback = cb;
  b->callback_ctx = ctx;
}

void Button_NotifyExti(Button *b) {
  if (b == NULL || b->port == NULL || !b->registered)
    return;
  uint32_t p = Lock();
  b->irq_active = Active(b);
  b->irq_tick = HAL_GetTick();
  b->irq_pending = 1U;
  if (b->edge_overflow || b->edge_count == BUTTON_EDGE_CAPACITY) {
    b->edge_overflow = 1U;
    if (b->overflow_count != UINT32_MAX)
      ++b->overflow_count;
  } else {
    unsigned tail = (b->edge_head + b->edge_count) % BUTTON_EDGE_CAPACITY;
    b->edges[tail] = (ButtonEdge){b->irq_tick, b->irq_active};
    ++b->edge_count;
  }
  Unlock(p);
}

void Button_NotifyExtiPin(uint16_t pin) {
  uint32_t p = Lock();
  for (unsigned i = 0; i < BUTTON_MAX_INSTANCES; ++i) {
    if (s_instances[i] != NULL && s_instances[i]->pin == pin) {
      Button_NotifyExti(s_instances[i]);
      break;
    }
  }
  Unlock(p);
}

/* Called while IRQs are masked. No user callbacks, delays or I/O except the
 * single GPIO sample at a due final deadline. Replay uses recorded edge times.
 */
static ButtonEvent Advance(Button *b, uint32_t now, uint8_t sample_gpio) {
  if (b->phase == BUTTON_PHASE_DEBOUNCE) {
    if (Remaining(now, b->debounce_tick, b->debounce_ms) != 0U)
      return BUTTON_EVENT_NONE;
    if (sample_gpio) {
      const uint8_t active_now = Active(b);
      if (active_now != b->debounce_active) {
        b->debounce_active = active_now;
        b->debounce_tick = now;
        return BUTTON_EVENT_NONE;
      }
    }
    uint8_t was_active = b->stable_state == b->active_state;
    b->phase = b->debounce_active ? BUTTON_PHASE_HELD : BUTTON_PHASE_IDLE;
    if (b->debounce_active != was_active) {
      b->stable_state = State(b, b->debounce_active);
      if (b->debounce_active) {
        b->press_tick = b->debounce_tick;
        b->extra_long_emitted = 0U;
        b->suppress_release_event = 0U;
      } else {
        if (b->suppress_release_event || b->extra_long_emitted) {
          b->suppress_release_event = 0U;
          b->extra_long_emitted = 0U;
          return BUTTON_EVENT_NONE;
        }
        uint32_t duration = b->debounce_tick - b->press_tick;
        return duration >= BUTTON_EXTRA_LONG_MS ? BUTTON_EVENT_EXTRA_LONG
               : duration >= BUTTON_LONG_MS     ? BUTTON_EVENT_LONG
                                                : BUTTON_EVENT_SHORT;
      }
    }
  }
  if (b->phase == BUTTON_PHASE_HELD && !b->suppress_release_event && !b->extra_long_emitted &&
      Remaining(now, b->press_tick, BUTTON_EXTRA_LONG_MS) == 0U) {
    if (sample_gpio && !Active(b)) {
      /* Lost hardware edge: fail conservatively, never manufacture extra-long. */
      b->phase = BUTTON_PHASE_DEBOUNCE;
      b->debounce_active = 0U;
      b->debounce_tick = now;
      b->suppress_release_event = 1U;
      return BUTTON_EVENT_NONE;
    }
    b->extra_long_emitted = 1U;
    return BUTTON_EVENT_EXTRA_LONG;
  }
  return BUTTON_EVENT_NONE;
}

ButtonEvent Button_Process(Button *b) {
  if (b == NULL || b->port == NULL)
    return BUTTON_EVENT_NONE;
  ButtonEvent event = BUTTON_EVENT_NONE;
  uint32_t p = Lock();
  if (b->edge_overflow) {
    uint8_t active = Active(b);
    b->edge_count = b->edge_head = b->edge_overflow = b->irq_pending = 0U;
    /* Remain logically pressed until a stable release is observed. A release
     * bounce following overflow must not become a new destructive gesture. */
    b->stable_state = b->active_state;
    b->suppress_release_event = 1U;
    b->extra_long_emitted = 0U;
    b->phase = BUTTON_PHASE_DEBOUNCE;
    b->debounce_active = active;
    b->debounce_tick = HAL_GetTick();
    Unlock(p);
    return BUTTON_EVENT_NONE;
  }
  while (b->edge_count) {
    ButtonEdge edge = b->edges[b->edge_head];
    /* Finish a previous stable interval BEFORE applying the next edge. A
     * delayed consumer therefore does not lose a complete press/release. */
    event = Advance(b, edge.tick, 0U);
    if (event != BUTTON_EVENT_NONE)
      break;
    b->edge_head = (uint8_t)((b->edge_head + 1U) % BUTTON_EDGE_CAPACITY);
    --b->edge_count;
    b->debounce_tick = edge.tick;
    b->debounce_active = edge.active;
    b->phase = BUTTON_PHASE_DEBOUNCE;
  }
  b->irq_pending = b->edge_count != 0U;
  if (event == BUTTON_EVENT_NONE)
    event = Advance(b, HAL_GetTick(), 1U);
  ButtonEventCallback callback = b->callback;
  void *context = b->callback_ctx;
  Unlock(p);
  if (event != BUTTON_EVENT_NONE && callback != NULL)
    callback(context, event);
  return event;
}

uint32_t Button_NextWakeDelay(const Button *b, uint32_t now) {
  if (b == NULL || b->port == NULL)
    return BUTTON_WAIT_FOREVER;
  uint32_t p = Lock(), delay = BUTTON_WAIT_FOREVER;
  if (b->irq_pending || b->edge_overflow)
    delay = 0U;
  else if (b->phase == BUTTON_PHASE_DEBOUNCE)
    delay = Remaining(now, b->debounce_tick, b->debounce_ms);
  else if (b->phase == BUTTON_PHASE_HELD && !b->suppress_release_event &&
           !b->extra_long_emitted)
    delay = Remaining(now, b->press_tick, BUTTON_EXTRA_LONG_MS);
  Unlock(p);
  return delay;
}
uint8_t Button_IsPressed(const Button *b) {
  return b != NULL && b->port != NULL && b->stable_state == b->active_state;
}
uint8_t Button_IsRegistered(const Button *b) {
  return b != NULL && b->registered;
}
uint32_t Button_OverflowCount(const Button *b) {
  return b != NULL ? b->overflow_count : 0U;
}
