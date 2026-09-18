#ifndef BUTTON_TEST_MAIN_H
#define BUTTON_TEST_MAIN_H

#include <stdint.h>

typedef struct GPIO_TypeDef {
  uint32_t unused;
} GPIO_TypeDef;

typedef enum {
  GPIO_PIN_RESET = 0,
  GPIO_PIN_SET = 1
} GPIO_PinState;

uint32_t HAL_GetTick(void);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);

#endif /* BUTTON_TEST_MAIN_H */
