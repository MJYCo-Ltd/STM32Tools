/*
 * Auxiliary.c
 *
 *  Created on: Apr 12, 2024
 *      Author: yty
 */
#include <string.h>
#include "main.h"
#include "cmsis_os.h"
#include "UartReceive.h"

/// 发送信息给串口
void SendDebugInfo(const unsigned char *pData, uint16_t uLength) {
  if (GetUartCount() < 1)
    return;
  UART_HandleTypeDef *pHUart = GetUart(1);
  while (HAL_OK != HAL_UART_Transmit_DMA(pHUart, pData, uLength)) {
    osDelay(1);
  }
  UpdateUartSendInfo(pHUart, uLength);
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
/// 让板子休眠
void SetBoardSleep() {
  HAL_SuspendTick();
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  HAL_ResumeTick();
}

/// 更改模型
void ChangeMode(STM_BOARD_STATE emState) {
  switch (emState) {
  case SLEEP_YTY:
    break;
  }
}

extern uint8_t GetCPUUsage(void);

/// 获取状态
STMSTATUS GetStatus(void) {
  static STMSTATUS S_LOCAL = {0, 0, 0, 0};
  S_LOCAL.unRamFree = xPortGetFreeHeapSize();
  //	S_LOCAL.unCPURate = GetCPUUsage();
  return (S_LOCAL);
}
