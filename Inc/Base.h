#ifndef STM32TOOLS_BASE_H
#define STM32TOOLS_BASE_H
#ifdef USE_FREERTOS
#include "FreeRTOS.h"
#include "cmsis_os.h"
#define YTY_DELAY_MS(ms) osDelay(ms) /* legacy: requires a 1 kHz kernel tick */
#define YTY_MALLOC(size) pvPortMalloc(size)
#define YTY_FREE(ptr) vPortFree(ptr)
#else
#include <stdlib.h>
#define YTY_DELAY_MS(ms) HAL_Delay(ms)
#define YTY_MALLOC(size) malloc(size)
#define YTY_FREE(ptr) free(ptr)
#endif
/* Keep legacy platform selection for clients outside component-target builds. */
#if defined(STM32F0xx) || defined(STM32F1xx) || defined(STM32F2xx) || \
    defined(STM32F3xx) || defined(STM32F4xx) || defined(STM32F7xx) || \
    defined(STM32G0xx) || defined(STM32G4xx) || defined(STM32H7xx) || \
    defined(STM32L0xx) || defined(STM32L1xx) || defined(STM32L4xx) || \
    defined(STM32L5xx) || defined(STM32WBxx) || defined(STM32WLxx) || \
    defined(STM32L476xx) || defined(STM32F411xE)
#ifndef PLATFORM_STM32
#define PLATFORM_STM32
#endif
#endif
#endif
