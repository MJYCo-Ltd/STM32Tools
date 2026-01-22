/*
 * Auxiliary.c
 *
 *  Created on: Apr 12, 2024
 *      Author: yty
 */
#include "Auxiliary.h"
#include "Base.h"
#include "UartReceive.h"
#include "main.h"
#include <string.h>

volatile uint8_t rtc_5min_flag = 0;
void SystemClock_Config(void);

// TickType_t g_base;

STMSTATUS G_LOCAL = {0, 0, 0, 0};

/// 发送信息给串口
void SendDebugInfo(const uint8_t *pData, uint16_t uLength) {
  if (GetUartCount() < 1)
    return;
  UART_HandleTypeDef *pHUart = GetUart(1);
  while (HAL_OK != HAL_UART_Transmit(pHUart, pData, uLength, 30)) {
    YTY_DELAY_MS(1);
  }
  UpdateUartSendInfo(pHUart, uLength);
}

extern TIM_HandleTypeDef htim10;
unsigned long g_TotalTime = 0;
void configureTimerForRunTimeStats(void) {
  HAL_TIM_Base_Start_IT(&htim10);
  g_TotalTime = 0;
}

unsigned long getRunTimeCounterValue(void) {
  return (g_TotalTime + __HAL_TIM_GET_COUNTER(&htim10));
}

/// 请求空间
void *RequestSpace(size_t unSize) {
  void *pBuffer = YTY_MALLOC(unSize);
  if (NULL != pBuffer) {
    memset(pBuffer, 0, unSize);
  }

  return (pBuffer);
}

/// 回收空间
void RecycleSpace(void *pBuffer) { YTY_FREE(pBuffer); }

/// 获取状态
STMSTATUS GetStatus(void) {
#ifdef USE_FREERTOS
  G_LOCAL.unRamFree = xPortGetFreeHeapSize();
  //	S_LOCAL.unCPURate = GetCPUUsage();
#endif
  return (G_LOCAL);
}

extern RTC_HandleTypeDef hrtc;

void EnterLowPowerMode(LOW_POWER_MODE mode, uint32_t WakeUpCounter,
                       uint32_t WakeUpClock) {
  /* 清除 PWR 唤醒标志 */
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

  /* 关闭旧的 RTC WakeUp Timer */
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

  /* 配置 RTC WakeUp Timer */
  HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, WakeUpCounter, WakeUpClock);

  /* 清除 RTC WakeUp 标志 */
  __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);

  switch (mode) {
  case LP_MODE_STOP:
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    SystemClock_Config();
    break;

  case LP_MODE_STANDBY:
    HAL_PWR_EnterSTANDBYMode();
    break;

  default:
    break;
  }
}
