/**
 ******************************************************************************
 * @file           : Button.h
 * @brief          : GPIO EXTI button driver with software debounce
 ******************************************************************************
 */
#ifndef STM32TOOLS_BUTTON_H
#define STM32TOOLS_BUTTON_H

#include "main.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Short press: held duration strictly less than this. */
#define BUTTON_LONG_MS 1000U
/** Extra-long press: held duration greater than or equal to this. */
#define BUTTON_EXTRA_LONG_MS 5000U

typedef enum {
  BUTTON_EVENT_NONE = 0,
  BUTTON_EVENT_SHORT,      /**< < 1 s, reported on release */
  BUTTON_EVENT_LONG,       /**< >= 1 s and < 5 s, reported on release */
  BUTTON_EVENT_EXTRA_LONG  /**< >= 5 s, reported when the threshold is reached */
} ButtonEvent;

typedef void (*ButtonEventCallback)(void *ctx, ButtonEvent event);

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState active_state;
  uint32_t debounce_ms;
  volatile uint8_t irq_pending;
  volatile uint32_t irq_tick;
  uint8_t phase;
  uint8_t extra_long_emitted;
  uint32_t press_tick;
  uint32_t debounce_tick;
  GPIO_PinState stable_state;
  ButtonEventCallback callback;
  void *callback_ctx;
} Button;

/**
 * Initialize software state. GPIO must already be EXTI falling (pull-up for
 * active-low keys). Call from task context after pin configuration.
 */
void Button_Init(Button *button, GPIO_TypeDef *port, uint16_t pin,
                 GPIO_PinState active_state, uint32_t debounce_ms);

void Button_SetCallback(Button *button, ButtonEventCallback callback,
                        void *ctx);

/**
 * ISR entry: wake/mark only. Call from HAL_GPIO_EXTI_Callback / EXTI IRQ.
 * Does not debounce, time the press, or invoke callbacks.
 */
void Button_NotifyExti(Button *button);

/** Dispatch an EXTI pin to the matching registered button. ISR-safe. */
void Button_NotifyExtiPin(uint16_t pin);

/**
 * Task-context pump: software debounce and press-duration classification.
 * Idle keys do not sample GPIO until an EXTI mark arrives.
 */
ButtonEvent Button_Process(Button *button);

/** Last confirmed logical pressed state; does not read GPIO. */
uint8_t Button_IsPressed(const Button *button);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_BUTTON_H */
