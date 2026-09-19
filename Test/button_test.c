#include <Button.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static GPIO_TypeDef g_port;
static GPIO_PinState g_pin = GPIO_PIN_SET;
static uint32_t g_tick;
static uint32_t g_primask;
static unsigned int g_reads;
static ButtonEvent g_callback_event;
static unsigned int g_callback_calls;
static Button *g_injected_button;
static uint8_t g_inject_exti_on_tick;

uint32_t HAL_GetTick(void)
{
  const uint32_t value = g_tick;
  if ((g_inject_exti_on_tick != 0U) && (g_injected_button != NULL)) {
    g_inject_exti_on_tick = 0U;
    ++g_tick;
    Button_NotifyExti(g_injected_button);
  }
  return value;
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
  (void)port;
  (void)pin;
  ++g_reads;
  return g_pin;
}

uint32_t __get_PRIMASK(void)
{
  return g_primask;
}

void __disable_irq(void)
{
  g_primask = 1U;
}

void __set_PRIMASK(uint32_t value)
{
  g_primask = value;
}

static void capture_event(void *ctx, ButtonEvent event)
{
  unsigned int *calls = ctx;
  ++(*calls);
  g_callback_event = event;
}

static void edge_at(Button *button, uint32_t tick, GPIO_PinState state)
{
  g_tick = tick;
  g_pin = state;
  Button_NotifyExti(button);
}

static void press_at(Button *button, uint32_t tick)
{
  edge_at(button, tick, GPIO_PIN_RESET);
}

static void release_at(Button *button, uint32_t tick)
{
  edge_at(button, tick, GPIO_PIN_SET);
}

static void init_idle(Button *button)
{
  memset(button, 0, sizeof(*button));
  g_pin = GPIO_PIN_SET;
  g_tick = 0U;
  g_primask = 0U;
  g_reads = 0U;
  g_callback_event = BUTTON_EVENT_NONE;
  g_callback_calls = 0U;
  g_injected_button = NULL;
  g_inject_exti_on_tick = 0U;
  Button_Init(button, &g_port, 1U, GPIO_PIN_RESET, 30U);
  assert(Button_IsRegistered(button) != 0U);
  Button_SetCallback(button, capture_event, &g_callback_calls);
}

static void finish(Button *button)
{
  Button_Deinit(button);
  assert(Button_IsRegistered(button) == 0U);
  assert(Button_IsPressed(button) == 0U);
}

static void confirm_press(Button *button, uint32_t pressed_at)
{
  press_at(button, pressed_at);
  assert(Button_Process(button) == BUTTON_EVENT_NONE);
  assert(Button_NextWakeDelay(button, pressed_at) == 30U);
  g_tick = pressed_at + 29U;
  assert(Button_Process(button) == BUTTON_EVENT_NONE);
  assert(Button_NextWakeDelay(button, g_tick) == 1U);
  g_tick = pressed_at + 30U;
  assert(Button_Process(button) == BUTTON_EVENT_NONE);
  assert(Button_IsPressed(button) == 1U);
}

static void test_idle_does_not_poll_gpio(void)
{
  Button button;
  unsigned int reads_after_init;

  init_idle(&button);
  reads_after_init = g_reads;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(Button_NextWakeDelay(&button, g_tick) == BUTTON_WAIT_FOREVER);
  assert(g_reads == reads_after_init);
  finish(&button);
}

static void test_bounce_is_rejected(void)
{
  Button button;

  init_idle(&button);
  press_at(&button, 0U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  release_at(&button, 10U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(Button_NextWakeDelay(&button, 10U) == 30U);
  g_tick = 40U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(g_callback_calls == 0U);
  assert(Button_IsPressed(&button) == 0U);
  assert(Button_NextWakeDelay(&button, g_tick) == BUTTON_WAIT_FOREVER);
  finish(&button);
}

static void test_short_press_on_release(void)
{
  Button button;

  init_idle(&button);
  confirm_press(&button, 0U);
  assert(Button_NextWakeDelay(&button, 30U) ==
         BUTTON_EXTRA_LONG_MS - 30U);

  release_at(&button, 200U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(Button_NextWakeDelay(&button, 200U) == 30U);
  g_tick = 230U;
  assert(Button_Process(&button) == BUTTON_EVENT_SHORT);
  assert(g_callback_calls == 1U);
  assert(g_callback_event == BUTTON_EVENT_SHORT);
  assert(Button_IsPressed(&button) == 0U);
  finish(&button);
}

static void test_long_press_on_release(void)
{
  Button button;

  init_idle(&button);
  confirm_press(&button, 0U);
  release_at(&button, 1500U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 1530U;
  assert(Button_Process(&button) == BUTTON_EVENT_LONG);
  assert(g_callback_event == BUTTON_EVENT_LONG);
  finish(&button);
}

static void test_extra_long_while_held(void)
{
  Button button;

  init_idle(&button);
  confirm_press(&button, 0U);
  g_tick = 4999U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(Button_NextWakeDelay(&button, g_tick) == 1U);
  g_tick = 5000U;
  assert(Button_Process(&button) == BUTTON_EVENT_EXTRA_LONG);
  assert(g_callback_calls == 1U);
  assert(Button_NextWakeDelay(&button, g_tick) == BUTTON_WAIT_FOREVER);

  release_at(&button, 5100U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 5130U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(g_callback_calls == 1U);
  finish(&button);
}

static void test_extra_long_on_late_release_processing(void)
{
  Button button;

  init_idle(&button);
  confirm_press(&button, 0U);
  release_at(&button, 5200U);
  /* Chronological replay observes the 5 s deadline before the queued release.
   * It emits once immediately, not after an additional release debounce. */
  assert(Button_Process(&button) == BUTTON_EVENT_EXTRA_LONG);
  g_tick = 5230U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(g_callback_calls == 1U);
  finish(&button);
}

static void test_stable_hold_does_not_poll_gpio(void)
{
  Button button;
  unsigned int reads_after_confirm;

  init_idle(&button);
  confirm_press(&button, 0U);
  reads_after_confirm = g_reads;
  g_tick = 1000U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 4000U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(g_reads == reads_after_confirm);
  finish(&button);
}

static void test_notify_by_pin_for_both_edges(void)
{
  Button button;

  init_idle(&button);
  g_tick = 0U;
  g_pin = GPIO_PIN_RESET;
  Button_NotifyExtiPin(1U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 30U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);

  g_tick = 80U;
  g_pin = GPIO_PIN_SET;
  Button_NotifyExtiPin(1U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 110U;
  assert(Button_Process(&button) == BUTTON_EVENT_SHORT);
  finish(&button);
}

static void test_exti_between_snapshot_and_now_does_not_underflow(void)
{
  Button button;

  init_idle(&button);
  g_pin = GPIO_PIN_RESET;
  g_tick = 100U;
  g_injected_button = &button;
  g_inject_exti_on_tick = 1U;

  /* EXTI arrives while Button_Process obtains now. It stays pending for the
   * next pass rather than combining an old now with a newer edge timestamp. */
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(g_callback_calls == 0U);
  assert(Button_IsPressed(&button) == 0U);
  assert(Button_NextWakeDelay(&button, g_tick) == 0U);

  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 131U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(Button_IsPressed(&button) == 1U);
  assert(g_callback_calls == 0U);
  finish(&button);
}

static void test_startup_held_release_updates_state_without_event(void)
{
  Button button;
  unsigned int reads_after_init;

  memset(&button, 0, sizeof(button));
  g_pin = GPIO_PIN_RESET;
  g_tick = 50U;
  g_primask = 0U;
  g_callback_event = BUTTON_EVENT_NONE;
  g_callback_calls = 0U;
  Button_Init(&button, &g_port, 1U, GPIO_PIN_RESET, 30U);
  assert(Button_IsRegistered(&button) != 0U);
  Button_SetCallback(&button, capture_event, &g_callback_calls);
  assert(Button_IsPressed(&button) != 0U);
  assert(Button_NextWakeDelay(&button, g_tick) == BUTTON_WAIT_FOREVER);

  reads_after_init = g_reads;
  g_tick = 6000U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(g_reads == reads_after_init);

  release_at(&button, 6010U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 6040U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(Button_IsPressed(&button) == 0U);
  assert(g_callback_calls == 0U);
  finish(&button);
}

static void test_registry_capacity_and_deinit(void)
{
  Button buttons[BUTTON_MAX_INSTANCES + 1U];
  uint8_t i;

  memset(buttons, 0, sizeof(buttons));
  g_pin = GPIO_PIN_SET;
  g_tick = 0U;
  g_primask = 0U;
  for (i = 0U; i < BUTTON_MAX_INSTANCES; ++i) {
    Button_Init(&buttons[i], &g_port, (uint16_t)(1U << i), GPIO_PIN_RESET, 30U);
    assert(Button_IsRegistered(&buttons[i]) != 0U);
  }
  Button_Init(&buttons[BUTTON_MAX_INSTANCES], &g_port,
              (uint16_t)(1U << BUTTON_MAX_INSTANCES), GPIO_PIN_RESET, 30U);
  assert(Button_IsRegistered(&buttons[BUTTON_MAX_INSTANCES]) == 0U);

  for (i = 0U; i <= BUTTON_MAX_INSTANCES; ++i) {
    Button_Deinit(&buttons[i]);
  }
}

int main(void)
{
  test_idle_does_not_poll_gpio();
  test_bounce_is_rejected();
  test_short_press_on_release();
  test_long_press_on_release();
  test_extra_long_while_held();
  test_extra_long_on_late_release_processing();
  test_stable_hold_does_not_poll_gpio();
  test_notify_by_pin_for_both_edges();
  test_exti_between_snapshot_and_now_does_not_underflow();
  test_startup_held_release_updates_state_without_event();
  test_registry_capacity_and_deinit();
  puts("button dual-edge event-driven tests passed");
  return 0;
}
