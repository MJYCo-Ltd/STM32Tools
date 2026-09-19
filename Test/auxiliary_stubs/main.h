#ifndef AUX_TEST_MAIN_H
#define AUX_TEST_MAIN_H
#include <stdint.h>
typedef struct { unsigned id; } RTC_HandleTypeDef;
typedef struct { unsigned id; } UART_HandleTypeDef;
typedef enum { HAL_OK=0, HAL_ERROR=1 } HAL_StatusTypeDef;
#define PWR_MAINREGULATOR_ON 0U
#define PWR_SLEEPENTRY_WFI 0U
#define PWR_LOWPOWERREGULATOR_ON 1U
#define PWR_STOPENTRY_WFI 0U
#define PWR_FLAG_WU 0U
#define __HAL_PWR_CLEAR_FLAG(x) ((void)(x))
#define IS_RTC_WAKEUP_COUNTER(x) ((x)<=65535U)
#define IS_RTC_WAKEUP_CLOCK(x) ((x)<=6U)
void HAL_PWR_EnterSLEEPMode(uint32_t regulator,uint32_t entry);
void HAL_PWR_EnterSTOPMode(uint32_t regulator,uint32_t entry);
void HAL_PWR_EnterSTANDBYMode(void);
uint32_t HAL_RCC_GetHCLKFreq(void);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *, const uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_RTCEx_DeactivateWakeUpTimer(RTC_HandleTypeDef *);
HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer_IT(RTC_HandleTypeDef *,uint32_t,uint32_t);
#endif
