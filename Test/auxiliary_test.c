#include <Auxiliary.h>
#include "main.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
uint32_t SystemCoreClock = 480000000U;
static unsigned stop_calls, standby_calls, restore_calls, timer_calls, tx_calls;
static HAL_StatusTypeDef arm_result, disarm_result;
static AuxiliaryResult clock_result;
size_t xPortGetFreeHeapSize(void) {
  return 40960U;
}
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *u, const uint8_t *p, uint16_t n,
                                    uint32_t timeout) {
  assert(u && p && n && timeout == 30U);
  ++tx_calls;
  return HAL_OK;
}
HAL_StatusTypeDef HAL_RTCEx_DeactivateWakeUpTimer(RTC_HandleTypeDef *r) {
  assert(r);
  return disarm_result;
}
HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer_IT(RTC_HandleTypeDef *r, uint32_t n, uint32_t c) {
  assert(r && n == 4U && c == 0U);
  ++timer_calls;
  return arm_result;
}
void HAL_PWR_EnterSLEEPMode(uint32_t r, uint8_t e) {
  (void)r;
  (void)e;
}
void HAL_PWR_EnterSTOPMode(uint32_t r, uint8_t e) {
  (void)r;
  (void)e;
  ++stop_calls;
}
void HAL_PWR_EnterSTANDBYMode(void) {
  ++standby_calls;
}
static AuxiliaryResult Restore(void *p) {
  assert(p == &restore_calls);
  ++restore_calls;
  return clock_result;
}
int main(void) {
  RTC_HandleTypeDef rtc = {1};
  UART_HandleTypeDef uart = {2};
  AuxiliaryConfig config = {&rtc, &uart, Restore, &restore_calls};
  assert(GetStatus().unRamFree == 40960U);
  assert(GetStatus().unRamTotal == 65536U);
  assert(GetStatus().unCPUFrequency == 480U);
  assert(Auxiliary_EnterLowPower(LP_MODE_STOP, 4U, 0U) == AUXILIARY_ERR_UNCONFIGURED);
  assert(Auxiliary_SendDebug((const uint8_t *)"x", 1U) == AUXILIARY_ERR_UNCONFIGURED);
  Auxiliary_Configure(&config);
  assert(Auxiliary_EnterLowPower((LOW_POWER_MODE)99, 4U, 0U) == AUXILIARY_ERR_PARAM);
  assert(Auxiliary_EnterLowPower(LP_MODE_STOP, UINT32_MAX, 0U) == AUXILIARY_ERR_PARAM);
  assert(Auxiliary_EnterLowPower(LP_MODE_STOP, 4U, UINT32_MAX) == AUXILIARY_ERR_PARAM);
  assert(timer_calls == 0U);
  arm_result = HAL_ERROR;
  assert(Auxiliary_EnterLowPower(LP_MODE_STOP, 4U, 0U) == AUXILIARY_ERR_IO);
  assert(Auxiliary_EnterLowPower(LP_MODE_STANDBY, 4U, 0U) == AUXILIARY_ERR_IO);
  EnterLowPowerMode(LP_MODE_STOP, 4U, 0U);
  assert(stop_calls == 0U && standby_calls == 0U && restore_calls == 0U);
  arm_result = HAL_OK;
  disarm_result = HAL_ERROR;
  assert(Auxiliary_EnterLowPower(LP_MODE_STOP, 4U, 0U) == AUXILIARY_ERR_IO);
  assert(stop_calls == 0U);
  disarm_result = HAL_OK;
  assert(Auxiliary_EnterLowPower(LP_MODE_STOP, 4U, 0U) == AUXILIARY_OK);
  assert(stop_calls == 1U && restore_calls == 1U);
  clock_result = AUXILIARY_ERR_CLOCK;
  assert(Auxiliary_EnterLowPower(LP_MODE_STOP, 4U, 0U) == AUXILIARY_ERR_CLOCK);
  assert(Auxiliary_SendDebug((const uint8_t *)"x", 1U) == AUXILIARY_OK && tx_calls == 1U);
  uint8_t *p = RequestSpace(1024U);
  assert(p);
  for (unsigned i = 0; i < 1024U; ++i)
    assert(p[i] == 0U);
  RecycleSpace(p);
  RecycleSpace(NULL);
  assert(RequestSpace(0U) == NULL);
  Auxiliary_Configure(NULL);
  Enter_Stop();
  assert(stop_calls == 2U);
  puts("Auxiliary checked wakeup and wide statistics: passed");
  return 0;
}
