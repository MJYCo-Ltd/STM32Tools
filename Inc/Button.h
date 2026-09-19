#ifndef STM32TOOLS_BUTTON_H
#define STM32TOOLS_BUTTON_H
#include "main.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#ifndef BUTTON_MAX_INSTANCES
#define BUTTON_MAX_INSTANCES 4U
#endif
#ifndef BUTTON_EDGE_CAPACITY
#define BUTTON_EDGE_CAPACITY 16U
#endif
#if BUTTON_EDGE_CAPACITY < 2 || BUTTON_EDGE_CAPACITY > 255 || BUTTON_MAX_INSTANCES < 1 || BUTTON_MAX_INSTANCES > 255
#error "Button capacities must fit the bounded uint8_t queues"
#endif
#define BUTTON_WAIT_FOREVER UINT32_MAX
#define BUTTON_LONG_MS 1000U
#define BUTTON_EXTRA_LONG_MS 5000U

typedef enum {
  BUTTON_EVENT_NONE = 0, BUTTON_EVENT_SHORT, BUTTON_EVENT_LONG,
  BUTTON_EVENT_EXTRA_LONG
} ButtonEvent;
typedef void (*ButtonEventCallback)(void *ctx, ButtonEvent event);
typedef struct { uint32_t tick; uint8_t active; } ButtonEdge;
typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState active_state;
  uint32_t debounce_ms;
  ButtonEdge edges[BUTTON_EDGE_CAPACITY];
  volatile uint8_t edge_head, edge_tail, edge_count, overflow;
  volatile uint32_t overflow_count;
  uint8_t phase, debounce_active, extra_long_emitted, suppress_release_event;
  uint8_t registered;
  uint32_t press_tick, debounce_tick;
  GPIO_PinState stable_state;
  ButtonEventCallback callback;
  void *callback_ctx;
} Button;

/* The board configures BOTH edges first. Call all consumer/lifecycle functions
 * from ONE task; interrupt notification is safe. Borrowed object must outlive
 * registration. Call Deinit before a nonstatic object's lifetime ends.
 * The edge queue preserves a complete press/release during task scheduling
 * delays, up to its fixed capacity. Overflow suppresses the uncertain gesture
 * through release; it never fabricates a short/long event. */
void Button_Init(Button *button, GPIO_TypeDef *port, uint16_t pin,
                 GPIO_PinState active_state, uint32_t debounce_ms);
void Button_Deinit(Button *button);
void Button_SetCallback(Button *button, ButtonEventCallback callback, void *ctx);
void Button_NotifyExti(Button *button);
void Button_NotifyExtiPin(uint16_t pin);
ButtonEvent Button_Process(Button *button);
uint32_t Button_NextWakeDelay(const Button *button, uint32_t now_ms);
uint8_t Button_IsPressed(const Button *button);
uint8_t Button_IsRegistered(const Button *button);
uint32_t Button_OverflowCount(const Button *button);
#ifdef __cplusplus
}
#endif
#endif
