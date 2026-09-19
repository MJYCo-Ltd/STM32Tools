#include "Button.h"
#include <stddef.h>
#include <string.h>

#define IDLE 0U
#define DEBOUNCE 1U
#define HELD 2U
static Button *s_instances[BUTTON_MAX_INSTANCES];

static uint32_t Lock(void) { uint32_t saved = __get_PRIMASK(); __disable_irq(); return saved; }
static void Unlock(uint32_t saved) { __set_PRIMASK(saved); }
static uint8_t Active(const Button *b) { return HAL_GPIO_ReadPin(b->port, b->pin) == b->active_state; }
static GPIO_PinState Level(const Button *b, uint8_t active)
{ return active ? b->active_state : (b->active_state == GPIO_PIN_RESET ? GPIO_PIN_SET : GPIO_PIN_RESET); }
static uint32_t Remaining(uint32_t now, uint32_t start, uint32_t duration)
{ uint32_t elapsed = now - start; return elapsed >= duration ? 0U : duration - elapsed; }

void Button_Deinit(Button *b)
{
  uint32_t saved;
  unsigned i;
  if (!b) return;
  saved = Lock();
  for (i = 0U; i < BUTTON_MAX_INSTANCES; ++i) if (s_instances[i] == b) s_instances[i] = NULL;
  b->registered = 0U;
  b->port = NULL;
  b->edge_count = 0U;
  b->callback = NULL;
  b->callback_ctx = NULL;
  Unlock(saved);
}

void Button_Init(Button *b, GPIO_TypeDef *port, uint16_t pin,
                 GPIO_PinState active_state, uint32_t debounce_ms)
{
  uint32_t saved;
  unsigned i, free_slot = BUTTON_MAX_INSTANCES;
  if (!b) return;
  Button_Deinit(b);
  memset(b, 0, sizeof(*b));
  if (!port || !pin || (pin & (uint16_t)(pin - 1U)) ||
      (active_state != GPIO_PIN_RESET && active_state != GPIO_PIN_SET) ||
      debounce_ms > INT32_MAX) return;
  saved = Lock();
  for (i = 0U; i < BUTTON_MAX_INSTANCES; ++i) {
    if (s_instances[i] && s_instances[i]->pin == pin) { Unlock(saved); return; }
    if (!s_instances[i] && free_slot == BUTTON_MAX_INSTANCES) free_slot = i;
  }
  if (free_slot == BUTTON_MAX_INSTANCES) { Unlock(saved); return; }
  b->port = port; b->pin = pin; b->active_state = active_state;
  b->debounce_ms = debounce_ms;
  b->stable_state = HAL_GPIO_ReadPin(port, pin);
  if (b->stable_state == active_state) {
    b->phase = HELD;
    b->suppress_release_event = 1U;
  }
  s_instances[free_slot] = b;
  b->registered = 1U;
  Unlock(saved);
}

void Button_SetCallback(Button *b, ButtonEventCallback callback, void *ctx)
{ if (b) { b->callback = callback; b->callback_ctx = ctx; } }

void Button_NotifyExti(Button *b)
{
  uint32_t saved;
  if (!b || !b->port || !b->registered) return;
  saved = Lock();
  if (b->edge_count == BUTTON_EDGE_CAPACITY) {
    b->overflow = 1U;
    if (b->overflow_count != UINT32_MAX) ++b->overflow_count;
  } else {
    b->edges[b->edge_head].active = Active(b);
    b->edges[b->edge_head].tick = HAL_GetTick();
    b->edge_head = (uint8_t)((b->edge_head + 1U) % BUTTON_EDGE_CAPACITY);
    ++b->edge_count;
  }
  Unlock(saved);
}
void Button_NotifyExtiPin(uint16_t pin)
{
  unsigned i;
  for (i = 0U; i < BUTTON_MAX_INSTANCES; ++i) {
    Button *b = s_instances[i];
    if (b && b->pin == pin) { Button_NotifyExti(b); return; }
  }
}

/* Advance the debouncer up to the NEXT recorded edge, not merely task time.
 * A historical interval with no edges proves debounce stability; only the
 * final live deadline samples GPIO. Called in a short IRQ-masked section. */
static ButtonEvent Advance(Button *b, uint32_t now, uint8_t live)
{
  if (b->phase == DEBOUNCE) {
    uint8_t active = b->debounce_active;
    if (Remaining(now, b->debounce_tick, b->debounce_ms)) return BUTTON_EVENT_NONE;
    if (live && Active(b) != active) {
      b->debounce_active = !active; b->debounce_tick = now;
      return BUTTON_EVENT_NONE;
    }
    b->phase = active ? HELD : IDLE;
    if ((b->stable_state == b->active_state) != active) {
      b->stable_state = Level(b, active);
      if (active) {
        b->press_tick = b->debounce_tick;
        b->extra_long_emitted = b->suppress_release_event = 0U;
      } else {
        uint32_t duration = b->debounce_tick - b->press_tick;
        uint8_t suppress = b->suppress_release_event || b->extra_long_emitted;
        b->suppress_release_event = b->extra_long_emitted = 0U;
        if (suppress) return BUTTON_EVENT_NONE;
        return duration >= BUTTON_EXTRA_LONG_MS ? BUTTON_EVENT_EXTRA_LONG
             : duration >= BUTTON_LONG_MS ? BUTTON_EVENT_LONG : BUTTON_EVENT_SHORT;
      }
    }
  }
  if (b->phase == HELD && !b->suppress_release_event && !b->extra_long_emitted &&
      !Remaining(now, b->press_tick, BUTTON_EXTRA_LONG_MS)) {
    if (live && !Active(b)) {
      b->phase = DEBOUNCE; b->debounce_active = 0U; b->debounce_tick = now;
      return BUTTON_EVENT_NONE;
    }
    b->extra_long_emitted = 1U;
    return BUTTON_EVENT_EXTRA_LONG;
  }
  return BUTTON_EVENT_NONE;
}

ButtonEvent Button_Process(Button *b)
{
  uint32_t saved;
  unsigned budget;
  ButtonEvent event = BUTTON_EVENT_NONE;
  if (!b || !b->port) return event;
  saved = Lock();
  if (b->overflow) {
    uint8_t active = Active(b);
    b->edge_head = b->edge_tail = b->edge_count = b->overflow = 0U;
    /* Drop uncertainty through a debounced release, including release bounce. */
    b->stable_state = b->active_state;
    b->phase = DEBOUNCE;
    b->debounce_active = active;
    b->debounce_tick = HAL_GetTick();
    b->suppress_release_event = 1U;
    b->extra_long_emitted = 0U;
    Unlock(saved);
    return event;
  }
  for (budget = 0U; budget <= BUTTON_EDGE_CAPACITY; ++budget) {
    uint8_t pending = b->edge_count;
    uint32_t horizon = pending ? b->edges[b->edge_tail].tick : HAL_GetTick();
    event = Advance(b, horizon, pending == 0U);
    if (event != BUTTON_EVENT_NONE || !pending) break;
    b->debounce_tick = b->edges[b->edge_tail].tick;
    b->debounce_active = b->edges[b->edge_tail].active;
    b->edge_tail = (uint8_t)((b->edge_tail + 1U) % BUTTON_EDGE_CAPACITY);
    --b->edge_count;
    b->phase = DEBOUNCE;
  }
  Unlock(saved);
  if (event != BUTTON_EVENT_NONE && b->callback) b->callback(b->callback_ctx, event);
  return event;
}

uint32_t Button_NextWakeDelay(const Button *b, uint32_t now)
{
  if (!b || !b->port) return BUTTON_WAIT_FOREVER;
  if (b->edge_count || b->overflow) return 0U;
  if (b->phase == DEBOUNCE) return Remaining(now, b->debounce_tick, b->debounce_ms);
  if (b->phase == HELD && !b->extra_long_emitted && !b->suppress_release_event)
    return Remaining(now, b->press_tick, BUTTON_EXTRA_LONG_MS);
  return BUTTON_WAIT_FOREVER;
}
uint8_t Button_IsPressed(const Button *b)
{ return b && b->port && b->stable_state == b->active_state; }
uint8_t Button_IsRegistered(const Button *b) { return b && b->registered; }
uint32_t Button_OverflowCount(const Button *b) { return b ? b->overflow_count : 0U; }
