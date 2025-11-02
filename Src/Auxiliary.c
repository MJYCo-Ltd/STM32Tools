/*
 * Auxiliary.c
 *
 *  Created on: Apr 12, 2024
 *      Author: yty
 */
#include "UartReceive.h"
#include "cmsis_os.h"
#include "main.h"
#include <string.h>
// TickType_t g_base;
extern TIM_HandleTypeDef htim10;
STMSTATUS G_LOCAL = {0, 0, 0, 0};
/// 发送信息给串口
void SendDebugInfo(const unsigned char *pData, uint16_t uLength) {
  if (GetUartCount() < 1)
    return;
  UART_HandleTypeDef *pHUart = GetUart(1);
  while (HAL_OK != HAL_UART_Transmit_DMA(pHUart, pData, uLength)){
    osDelay(1);
  }
  UpdateUartSendInfo(pHUart, uLength);
}

void configureTimerForRunTimeStats(void) {
  __HAL_TIM_SET_COUNTER(&htim10, 0);
  HAL_TIM_Base_Start(&htim10);
}

unsigned long getRunTimeCounterValue(void) {
  return __HAL_TIM_GET_COUNTER(&htim10);
}

/// 请求空间
void *RequestSpace(size_t unSize) {
  void *pBuffer = pvPortMalloc(unSize);
  if (NULL != pBuffer) {
    memset(pBuffer, 0, unSize);
  }

  return (pBuffer);
}

/// 回收空间
void RecycleSpace(void *pBuffer) { vPortFree(pBuffer); }

/// 获取状态
STMSTATUS GetStatus(void) {
  G_LOCAL.unRamFree = xPortGetFreeHeapSize();
  //	S_LOCAL.unCPURate = GetCPUUsage();
  return (G_LOCAL);
}
