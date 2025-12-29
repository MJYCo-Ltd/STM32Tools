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

volatile uint16_t rtc_sec_cnt = 0;
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

extern UART_HandleTypeDef huart1;
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

void enter_stop_until_5min(void) {
  rtc_5min_flag = 0;

  HAL_SuspendTick(); // 关闭 SysTick 避免唤醒
  while (!rtc_5min_flag) {
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    // RTC 中断唤醒 MCU 执行到这里
    SystemClock_Config(); // 必须重配时钟
  }
  HAL_ResumeTick();
}
