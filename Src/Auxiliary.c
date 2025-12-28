/*
 * Auxiliary.c
 *
 *  Created on: Apr 12, 2024
 *      Author: yty
 */
#include <string.h>
#include "UartReceive.h"
#include "Base.h"
#include "main.h"
#include "Auxiliary.h"

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

#define FLASH_PARAM_PAGE_ADDR 0x0800FC00 // 最后一页
/// 保存数据到flash中
uint8_t SaveFlash(const uint8_t *pData, uint16_t unLength) {
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t PageError = 0;

  // Step 1：解锁 Flash
  HAL_FLASH_Unlock();

  // Step 2：擦除
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = FLASH_PARAM_PAGE_ADDR;
  EraseInitStruct.NbPages = 1;

  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
    // 擦除失败
    HAL_FLASH_Lock();
    return (0);
  }

  // Step 3：写入
  for (uint16_t i = 0; i < unLength; i += 4) {
    static uint32_t word;
    memcpy(&word, &pData[i], sizeof(word));

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_PARAM_PAGE_ADDR + i,
                          word) != HAL_OK) {
      HAL_FLASH_Lock();
      return (0);
    }
  }

  // Step 4：锁定
  HAL_FLASH_Lock();
  return (1);
}

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
