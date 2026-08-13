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

#define DEBUG_UART_MAX_ATTEMPTS 3U

volatile uint8_t rtc_5min_flag = 0;
void SystemClock_Config(void);

// TickType_t g_base;

STMSTATUS G_LOCAL = {0, 0, 0, 0};

/// 发送信息给串口
void SendDebugInfo(const uint8_t *pData, uint16_t uLength) {
  UART_HandleTypeDef *pHUart;
  uint32_t attempt;

  if ((pData == NULL) || (uLength == 0U) || (GetUartCount() < 1U)) {
    return;
  }
  pHUart = GetUart(1U);
  if (pHUart == NULL) {
    return;
  }
  for (attempt = 0U; attempt < DEBUG_UART_MAX_ATTEMPTS; ++attempt) {
    if (HAL_UART_Transmit(pHUart, pData, uLength, 30U) == HAL_OK) {
      UpdateUartSendInfo(pHUart, uLength);
      return;
    }
    YTY_DELAY_MS(1);
  }
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

/// 进入休眠模式
void Enter_Sleep() {
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

/// 进入停止模式
void Enter_Stop(void) {
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
  SystemClock_Config();
}

extern RTC_HandleTypeDef hrtc;

void EnterLowPowerMode(LOW_POWER_MODE mode, uint32_t WakeUpCounter,
                       uint32_t WakeUpClock) {
  /* 清除 PWR 唤醒标志 */
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

  /* 配置 RTC WakeUp Timer */
  HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, WakeUpCounter, WakeUpClock);

  switch (mode) {
  case LP_MODE_STOP:
    Enter_Stop();
    break;

  case LP_MODE_STANDBY:
    HAL_PWR_EnterSTANDBYMode();
    break;

  default:
    break;
  }
}
