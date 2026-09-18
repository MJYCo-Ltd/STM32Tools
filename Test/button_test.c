#include <Button.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static GPIO_TypeDef g_port;
static GPIO_PinState g_pin = GPIO_PIN_SET;
static uint32_t g_tick;
static unsigned int g_reads;
static ButtonEvent g_callback_event;
static unsigned int g_callback_calls;

uint32_t HAL_GetTick(void)
{
  return g_tick;
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
  (void)port;
  (void)pin;
  ++g_reads;
  return g_pin;
}

static void capture_event(void *ctx, ButtonEvent event)
{
  unsigned int *calls = ctx;
  ++(*calls);
  g_callback_event = event;
}

static void press_at(Button *button, uint32_t tick)
{
  g_tick = tick;
  g_pin = GPIO_PIN_RESET;
  Button_NotifyExti(button);
}

static void release_at(uint32_t tick)
{
  g_tick = tick;
  g_pin = GPIO_PIN_SET;
}

static void init_idle(Button *button)
{
  memset(button, 0, sizeof(*button));
  g_pin = GPIO_PIN_SET;
  g_tick = 0U;
  g_reads = 0U;
  g_callback_event = BUTTON_EVENT_NONE;
  g_callback_calls = 0U;
  Button_Init(button, &g_port, 1U, GPIO_PIN_RESET, 30U);
  Button_SetCallback(button, capture_event, &g_callback_calls);
}

static void test_idle_does_not_poll_gpio(void)
{
  Button button;
  unsigned int reads_after_init;

  init_idle(&button);
  reads_after_init = g_reads;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(g_reads == reads_after_init);
}

static void test_bounce_is_rejected(void)
{
  Button button;

  init_idle(&button);
  press_at(&button, 0U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  release_at(10U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 40U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(g_callback_calls == 0U);
  assert(Button_IsPressed(&button) == 0U);
}

static void test_short_press_on_release(void)
{
  Button button;

  init_idle(&button);
  press_at(&button, 0U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 30U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(Button_IsPressed(&button) == 1U);
  release_at(200U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 230U;
  assert(Button_Process(&button) == BUTTON_EVENT_SHORT);
  assert(g_callback_calls == 1U);
  assert(g_callback_event == BUTTON_EVENT_SHORT);
  assert(Button_IsPressed(&button) == 0U);
}

static void test_long_press_on_release(void)
{
  Button button;

  init_idle(&button);
  press_at(&button, 0U);
  g_tick = 30U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  release_at(1500U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 1530U;
  assert(Button_Process(&button) == BUTTON_EVENT_LONG);
  assert(g_callback_event == BUTTON_EVENT_LONG);
}

static void test_extra_long_while_held(void)
{
  Button button;

  init_idle(&button);
  press_at(&button, 0U);
  g_tick = 30U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 5000U;
  assert(Button_Process(&button) == BUTTON_EVENT_EXTRA_LONG);
  assert(g_callback_calls == 1U);
  release_at(5100U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 5130U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  assert(g_callback_calls == 1U);
}

static void test_extra_long_on_late_process(void)
{
  Button button;

  init_idle(&button);
  press_at(&button, 0U);
  g_tick = 30U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  release_at(5200U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 5230U;
  assert(Button_Process(&button) == BUTTON_EVENT_EXTRA_LONG);
}

static void test_notify_by_pin(void)
{
  Button button;

  init_idle(&button);
  g_tick = 0U;
  g_pin = GPIO_PIN_RESET;
  Button_NotifyExtiPin(1U);
  g_tick = 30U;
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  release_at(80U);
  assert(Button_Process(&button) == BUTTON_EVENT_NONE);
  g_tick = 110U;
  assert(Button_Process(&button) == BUTTON_EVENT_SHORT);
}

int main(void)
{
  test_idle_does_not_poll_gpio();
  test_bounce_is_rejected();
  test_short_press_on_release();
  test_long_press_on_release();
  test_extra_long_while_held();
  test_extra_long_on_late_process();
  test_notify_by_pin();
  puts("button exti debounce and duration tests passed");
  return 0;
}
