#include <Button.h>
#include <assert.h>
#include <stdio.h>
static GPIO_TypeDef port;
static uint32_t tick, primask;
static GPIO_PinState pin = GPIO_PIN_SET;
static unsigned reads, callbacks;
uint32_t HAL_GetTick(void) {
  return tick;
}
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *p, uint16_t n) {
  (void)p;
  (void)n;
  ++reads;
  return pin;
}
uint32_t __get_PRIMASK(void) {
  return primask;
}
void __disable_irq(void) {
  primask = 1U;
}
void __set_PRIMASK(uint32_t p) {
  primask = p;
}
static void Callback(void *p, ButtonEvent e) {
  (void)p;
  assert(primask == 0U && e != BUTTON_EVENT_NONE);
  ++callbacks;
}
static void Edge(Button *b, uint32_t t, GPIO_PinState s) {
  tick = t;
  pin = s;
  Button_NotifyExti(b);
}
static void Init(Button *b) {
  pin = GPIO_PIN_SET;
  tick = 0;
  Button_Init(b, &port, 1U, GPIO_PIN_RESET, 30U);
  assert(Button_IsRegistered(b));
  Button_SetCallback(b, Callback, NULL);
}
static void DelayedGesture(uint32_t start, uint32_t duration, ButtonEvent expect) {
  Button b;
  Init(&b);
  Edge(&b, start, GPIO_PIN_RESET);
  Edge(&b, start + duration, GPIO_PIN_SET);
  tick = start + duration + 30U;
  assert(Button_Process(&b) == expect);
  while (Button_NextWakeDelay(&b, tick) == 0U)
    assert(Button_Process(&b) == BUTTON_EVENT_NONE);
  tick += 30U;
  assert(Button_Process(&b) == BUTTON_EVENT_NONE);
  assert(!Button_IsPressed(&b));
  Button_Deinit(&b);
}
int main(void) {
  DelayedGesture(100U, 200U, BUTTON_EVENT_SHORT);
  DelayedGesture(100U, 1500U, BUTTON_EVENT_LONG);
  DelayedGesture(100U, 5200U, BUTTON_EVENT_EXTRA_LONG);
  DelayedGesture(UINT32_MAX - 100U, 200U, BUTTON_EVENT_SHORT);
  Button b;
  Init(&b);
  Edge(&b, 0U, GPIO_PIN_RESET);
  Edge(&b, 10U, GPIO_PIN_SET);
  tick = 40U;
  assert(Button_Process(&b) == BUTTON_EVENT_NONE);
  Edge(&b, 100U, GPIO_PIN_RESET);
  Edge(&b, 105U, GPIO_PIN_SET);
  Edge(&b, 110U, GPIO_PIN_RESET);
  tick = 140U;
  assert(Button_Process(&b) == BUTTON_EVENT_NONE && Button_IsPressed(&b));
  unsigned before = reads;
  tick = 200U;
  assert(Button_Process(&b) == BUTTON_EVENT_NONE && reads == before);
  Edge(&b, 5109U, GPIO_PIN_SET);
  tick = 5200U;
  assert(Button_Process(&b) == BUTTON_EVENT_LONG);
  Button_Deinit(&b);
  Init(&b);
  Edge(&b, 0U, GPIO_PIN_RESET);
  tick = 30;
  assert(Button_Process(&b) == BUTTON_EVENT_NONE);
  tick = 5000U;
  assert(Button_Process(&b) == BUTTON_EVENT_EXTRA_LONG);
  tick = 6000U;
  assert(Button_Process(&b) == BUTTON_EVENT_NONE);
  Edge(&b, 6100U, GPIO_PIN_SET);
  tick = 6130;
  assert(Button_Process(&b) == BUTTON_EVENT_NONE);
  for (unsigned i = 0; i < BUTTON_EDGE_CAPACITY + 3U; ++i)
    Edge(&b, 7000U + i, (i & 1U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  assert(Button_OverflowCount(&b) > 0U);
  tick = 8000;
  assert(Button_Process(&b) == BUTTON_EVENT_NONE);
  Edge(&b, 8100U, GPIO_PIN_SET);
  tick = 8130;
  assert(Button_Process(&b) == BUTTON_EVENT_NONE);
  Edge(&b, 8200U, GPIO_PIN_RESET);
  Edge(&b, 8400U, GPIO_PIN_SET);
  tick = 8430;
  assert(Button_Process(&b) == BUTTON_EVENT_SHORT);
  Button_Deinit(&b);
  /* A finite burst with two complete gestures must produce two ordered events. */
  Init(&b);
  Edge(&b, 100, GPIO_PIN_RESET);
  Edge(&b, 200, GPIO_PIN_SET);
  Edge(&b, 300, GPIO_PIN_RESET);
  Edge(&b, 400, GPIO_PIN_SET);
  tick = 500;
  assert(Button_Process(&b) == BUTTON_EVENT_SHORT);
  assert(Button_NextWakeDelay(&b, tick) == 0U);
  assert(Button_Process(&b) == BUTTON_EVENT_SHORT);
  assert(Button_Process(&b) == BUTTON_EVENT_NONE);
  Button_Deinit(&b);
  pin = GPIO_PIN_RESET;
  Button_Init(&b, &port, 1U, GPIO_PIN_RESET, 30U);
  tick = 9000;
  assert(Button_Process(&b) == BUTTON_EVENT_NONE);
  Edge(&b, 9100, GPIO_PIN_SET);
  tick = 9130;
  assert(Button_Process(&b) == BUTTON_EVENT_NONE);
  Button_Deinit(&b);
  assert(primask == 0U);
  printf("Button ordered history/overflow/wrap/startup: passed (%u callbacks)\n", callbacks);
  return 0;
}
