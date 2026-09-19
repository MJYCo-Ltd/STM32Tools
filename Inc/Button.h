/**
 ******************************************************************************
 * @file           : Button.h
 * @brief          : GPIO dual-edge EXTI button driver with software debounce
 ******************************************************************************
 */
#ifndef STM32TOOLS_BUTTON_H
#define STM32TOOLS_BUTTON_H

#include "main.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum buttons dispatched by Button_NotifyExtiPin(). */
#ifndef BUTTON_MAX_INSTANCES
#define BUTTON_MAX_INSTANCES 4U
#endif

/** Bounded edge history. Overflow discards the gesture and resynchronizes,
 * never manufactures a click/long press. RAM cost is capacity * sizeof(edge).
 */
#ifndef BUTTON_EDGE_CAPACITY
#define BUTTON_EDGE_CAPACITY 16U
#endif
#if BUTTON_EDGE_CAPACITY < 2U || BUTTON_EDGE_CAPACITY > 255U
#error "BUTTON_EDGE_CAPACITY must be 2..255"
#endif

typedef struct {
  uint32_t tick;
  uint8_t active;
} ButtonEdge;

/** Returned by Button_NextWakeDelay() when no timer wake is required. */
#define BUTTON_WAIT_FOREVER UINT32_MAX

/** Short press: held duration strictly less than this. */
#define BUTTON_LONG_MS 1000U
/** Extra-long press: held duration greater than or equal to this. */
#define BUTTON_EXTRA_LONG_MS 5000U

typedef enum {
  BUTTON_EVENT_NONE = 0,
  BUTTON_EVENT_SHORT,     /**< < 1 s, reported on release */
  BUTTON_EVENT_LONG,      /**< >= 1 s and < 5 s, reported on release */
  BUTTON_EVENT_EXTRA_LONG /**< >= 5 s, reported when threshold/release is handled */
} ButtonEvent;

typedef void (*ButtonEventCallback)(void *ctx, ButtonEvent event);

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState active_state;
  uint32_t debounce_ms;
  volatile uint8_t irq_pending;
  volatile uint8_t irq_active;
  volatile uint32_t irq_tick;
  uint8_t phase;
  uint8_t debounce_active;
  uint8_t extra_long_emitted;
  uint8_t suppress_release_event;
  uint8_t registered;
  uint32_t press_tick;
  uint32_t debounce_tick;
  GPIO_PinState stable_state;
  ButtonEdge edges[BUTTON_EDGE_CAPACITY];
  volatile uint8_t edge_head, edge_count, edge_overflow;
  volatile uint32_t overflow_count;
  ButtonEventCallback callback;
  void *callback_ctx;
} Button;

/**
 * Initialize software state and register pin dispatch when capacity permits.
 * GPIO must already be configured for rising+falling EXTI. Call from task
 * context after pin configuration. If the key is held during init, its release
 * is tracked but no short/long event is emitted for that startup hold.
 */
void Button_Init(Button *button, GPIO_TypeDef *port, uint16_t pin, GPIO_PinState active_state,
                 uint32_t debounce_ms);

/**
 * Remove pin dispatch and invalidate the object. Required before a non-static
 * Button object leaves scope. Call from task context, never from an ISR.
 */
void Button_Deinit(Button *button);

void Button_SetCallback(Button *button, ButtonEventCallback callback, void *ctx);

/**
 * ISR entry for either GPIO edge. Captures the current logical level and tick;
 * it does not debounce or invoke callbacks.
 */
void Button_NotifyExti(Button *button);

/**
 * Dispatch an EXTI pin to the matching registered button. ISR-safe. Pins must
 * be unique among registered buttons because HAL's callback has no port.
 */
void Button_NotifyExtiPin(uint16_t pin);

/**
 * Task-context state-machine pump. Call after EXTI wake or after the delay
 * returned by Button_NextWakeDelay(). A delayed task replays bounded edge
 * history chronologically. Returns at most one event per call; call again when
 * NextWakeDelay returns zero. One task owns Process/Init/Deinit and callbacks.
 * Time between processing opportunities must be less than 2^31 milliseconds.
 * Stable GPIO is sampled only at a debounce/extra-long deadline, not polled.
 */
ButtonEvent Button_Process(Button *button);

/**
 * Milliseconds until Button_Process() must run again. Returns 0 for pending
 * work and BUTTON_WAIT_FOREVER when only a future EXTI can make progress.
 */
uint32_t Button_NextWakeDelay(const Button *button, uint32_t now_ms);

/** Last confirmed logical pressed state; does not read GPIO. */
uint8_t Button_IsPressed(const Button *button);

/** Non-zero when Button_NotifyExtiPin() can dispatch to this object. */
uint8_t Button_IsRegistered(const Button *button);
uint32_t Button_OverflowCount(const Button *button);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_BUTTON_H */
