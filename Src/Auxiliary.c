/*
 * Auxiliary.c
 *
 *  Created on: Apr 12, 2024
 *      Author: yty
 */
#include "Auxiliary.h"
#include "Base.h"
#include "main.h"
#include <string.h>

static AuxiliaryConfig s_config;
static AuxiliaryStatus s_last_error;

AuxiliaryStatus Auxiliary_LastError(void) { return s_last_error; }

void Auxiliary_Configure(const AuxiliaryConfig *config) {
  if (config == NULL)
    memset(&s_config, 0, sizeof(s_config));
  else
    s_config = *config;
  s_last_error = AUXILIARY_OK;
}

AuxiliaryResult Auxiliary_SendDebug(const uint8_t *data, uint16_t length) {
  if (data == NULL || length == 0U)
    return AUXILIARY_ERR_PARAM;
  if (s_config.debug_uart == NULL)
    return AUXILIARY_ERR_UNCONFIGURED;
  /* A single bounded attempt: a partial transmission must not be retried as
   * though it were a new complete debug record. No implicit modem UART use. */
  return HAL_UART_Transmit((UART_HandleTypeDef *)s_config.debug_uart, data, length, 30U) ==
                 HAL_OK
             ? AUXILIARY_OK
             : AUXILIARY_ERR_IO;
}

void SendDebugInfo(const uint8_t *data, uint16_t length) {
  (void)Auxiliary_SendDebug(data, length);
}

void *RequestSpace(size_t bytes) {
  void *buffer;
  if (bytes == 0U)
    return NULL;
  buffer = YTY_MALLOC(bytes);
  if (buffer != NULL)
    memset(buffer, 0, bytes);
  return buffer;
}

void RecycleSpace(void *buffer) {
  if (buffer != NULL)
    YTY_FREE(buffer);
}

STMSTATUS GetStatus(void) {
  STMSTATUS status = {0};
#ifdef USE_FREERTOS
  status.unRamFree = xPortGetFreeHeapSize();
#ifdef configTOTAL_HEAP_SIZE
  status.unRamTotal = configTOTAL_HEAP_SIZE;
#endif
  status.valid_fields |= STMSTATUS_HEAP_VALID;
#endif
  status.unCPUFrequency = SystemCoreClock / 1000000U;
  status.valid_fields |= STMSTATUS_CPU_FREQUENCY_VALID;
  return status;
}

void Enter_Sleep(void) {
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

AuxiliaryResult Auxiliary_EnterStop(void) {
  if (s_config.restore_clock == NULL)
    return s_last_error = AUXILIARY_ERR_UNCONFIGURED;
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
  return s_last_error = s_config.restore_clock(s_config.context);
}

AuxiliaryResult Auxiliary_EnterLowPower(LOW_POWER_MODE mode, uint32_t counter, uint32_t clock) {
  RTC_HandleTypeDef *rtc = s_config.rtc;
  AuxiliaryResult result;
  if (mode != LP_MODE_STOP && mode != LP_MODE_STANDBY)
    return s_last_error = AUXILIARY_ERR_PARAM;
  if (counter > UINT16_MAX || !IS_RTC_WAKEUP_CLOCK(clock))
    return s_last_error = AUXILIARY_ERR_PARAM;
  if (rtc == NULL || (mode == LP_MODE_STOP && s_config.restore_clock == NULL))
    return s_last_error = AUXILIARY_ERR_UNCONFIGURED;
  if (HAL_RTCEx_DeactivateWakeUpTimer(rtc) != HAL_OK)
    return s_last_error = AUXILIARY_ERR_IO;
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
  if (HAL_RTCEx_SetWakeUpTimer_IT(rtc, counter, clock) != HAL_OK)
    return s_last_error = AUXILIARY_ERR_IO;
  if (mode == LP_MODE_STANDBY) {
    HAL_PWR_EnterSTANDBYMode(); /* normally wakes via reset; does not return */
    (void)HAL_RTCEx_DeactivateWakeUpTimer(rtc);
    return s_last_error = AUXILIARY_ERR_IO;
  }
  result = Auxiliary_EnterStop();
  if (HAL_RTCEx_DeactivateWakeUpTimer(rtc) != HAL_OK && result == AUXILIARY_OK)
    result = AUXILIARY_ERR_IO;
  return s_last_error = result;
}

void Enter_Stop(void) {
  (void)Auxiliary_EnterStop();
}
void EnterLowPowerMode(LOW_POWER_MODE mode, uint32_t counter, uint32_t clock) {
  (void)Auxiliary_EnterLowPower(mode, counter, clock);
}
