#include "Auxiliary.h"
#include "Base.h"
#include "main.h"
#include <string.h>

static AuxiliaryConfig s_config;
static AuxiliaryStatus s_last_error;
STMSTATUS G_LOCAL = {0};

void Auxiliary_Configure(const AuxiliaryConfig *config)
{
  if (config) s_config = *config;
  else memset(&s_config, 0, sizeof(s_config));
  s_last_error = AUXILIARY_OK;
}

void SendDebugInfo(const uint8_t *data, uint16_t length)
{
  uint32_t attempt;
  if (!data || !length || !s_config.debug_uart) return;
  /* A dedicated debug UART is supplied by the board, not inferred from module
   * registration order. Do not bind a modem/fieldbus UART to this endpoint. */
  for (attempt = 0U; attempt < 3U; ++attempt) {
    if (HAL_UART_Transmit((UART_HandleTypeDef *)s_config.debug_uart,
                         data, length, 30U) == HAL_OK) return;
    YTY_DELAY_MS(1U);
  }
}

void *RequestSpace(size_t size)
{
  void *buffer = YTY_MALLOC(size);
  if (buffer) memset(buffer, 0, size);
  return buffer;
}
void RecycleSpace(void *buffer) { YTY_FREE(buffer); }

STMSTATUS GetStatus(void)
{
#ifdef USE_FREERTOS
  G_LOCAL.unRamFree = xPortGetFreeHeapSize();
#ifdef configTOTAL_HEAP_SIZE
  G_LOCAL.unRamTotal = configTOTAL_HEAP_SIZE;
#endif
#endif
  G_LOCAL.unCPUFrequency = HAL_RCC_GetHCLKFreq() / 1000000U;
  return G_LOCAL;
}

void Enter_Sleep(void)
{
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

AuxiliaryStatus Auxiliary_EnterStop(void)
{
  if (!s_config.restore_clock) return s_last_error = AUXILIARY_ERR_UNCONFIGURED;
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
  return s_last_error = s_config.restore_clock(s_config.context);
}

AuxiliaryStatus Auxiliary_EnterLowPower(LOW_POWER_MODE mode, uint32_t counter,
                                        uint32_t clock)
{
  RTC_HandleTypeDef *rtc = (RTC_HandleTypeDef *)s_config.rtc;
  AuxiliaryStatus status;
  if (mode != LP_MODE_STOP && mode != LP_MODE_STANDBY)
    return s_last_error = AUXILIARY_ERR_PARAM;
#ifdef IS_RTC_WAKEUP_COUNTER
  if (!IS_RTC_WAKEUP_COUNTER(counter)) return s_last_error = AUXILIARY_ERR_PARAM;
#endif
#ifdef IS_RTC_WAKEUP_CLOCK
  if (!IS_RTC_WAKEUP_CLOCK(clock)) return s_last_error = AUXILIARY_ERR_PARAM;
#endif
  if (!rtc || (mode == LP_MODE_STOP && !s_config.restore_clock))
    return s_last_error = AUXILIARY_ERR_UNCONFIGURED;
  if (HAL_RTCEx_DeactivateWakeUpTimer(rtc) != HAL_OK)
    return s_last_error = AUXILIARY_ERR_IO;
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
  if (HAL_RTCEx_SetWakeUpTimer_IT(rtc, counter, clock) != HAL_OK)
    return s_last_error = AUXILIARY_ERR_IO;
  if (mode == LP_MODE_STANDBY) {
    HAL_PWR_EnterSTANDBYMode();
    /* A successful Standby entry resumes via reset, not this return path. */
    (void)HAL_RTCEx_DeactivateWakeUpTimer(rtc);
    return s_last_error = AUXILIARY_ERR_IO;
  }
  status = Auxiliary_EnterStop();
  if (HAL_RTCEx_DeactivateWakeUpTimer(rtc) != HAL_OK && status == AUXILIARY_OK)
    status = AUXILIARY_ERR_IO;
  return s_last_error = status;
}

/* Compatibility wrappers retain their old signatures; errors are not converted
 * to success and can be inspected through Auxiliary_LastError(). */
void Enter_Stop(void) { (void)Auxiliary_EnterStop(); }
void EnterLowPowerMode(LOW_POWER_MODE mode, uint32_t counter, uint32_t clock)
{ (void)Auxiliary_EnterLowPower(mode, counter, clock); }
AuxiliaryStatus Auxiliary_LastError(void) { return s_last_error; }
