#ifndef AUXILIARY_TEST_MAIN_H
#define AUXILIARY_TEST_MAIN_H
#include <stdint.h>
typedef struct {
  int id;
} UART_HandleTypeDef;
typedef struct {
  int id;
} RTC_HandleTypeDef;
typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;
extern uint32_t SystemCoreClock;
#define PWR_MAINREGULATOR_ON 0U
#define PWR_LOWPOWERREGULATOR_ON 1U
#define PWR_SLEEPENTRY_WFI 0U
#define PWR_STOPENTRY_WFI 0U
#define PWR_FLAG_WU 1U
#define __HAL_PWR_CLEAR_FLAG(flag) ((void)(flag))
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *, const uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_RTCEx_DeactivateWakeUpTimer(RTC_HandleTypeDef *);
HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer_IT(RTC_HandleTypeDef *, uint32_t, uint32_t);
void HAL_PWR_EnterSLEEPMode(uint32_t, uint8_t);
void HAL_PWR_EnterSTOPMode(uint32_t, uint8_t);
void HAL_PWR_EnterSTANDBYMode(void);
#endif

#ifndef IS_RTC_WAKEUP_CLOCK
#define IS_RTC_WAKEUP_CLOCK(clock) ((clock) <= 6U)
#endif
