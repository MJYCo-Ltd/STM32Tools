/*
 * UartReceive.c
 *
 *  Created on: Apr 10, 2024
 *      Author: yty
 */
#include <string.h>
#include "Base.h"
#include "UartReceive.h"

/// 定义串口数据结构体
typedef struct _Uart_Info {
  UART_HandleTypeDef *pHUart;    /// 串口句柄指针
  ReceiveUartCallback pCallback; /// 串口回调
  uint8_t *pBuffer;              /// 串口缓冲区
  uint8_t *pReceive;             /// 串口接收地址
#ifdef USE_FREERTOS
  osMessageQueueId_t hQueueId;   /// 串口获取消息队列
#endif
  IOInfo stAllIOInfo;            /// 串口收发数据统计
} Uart_Info;

static Uart_Info **pUartInfoArray = NULL;
const uint16_t UART_BUFFER_LENGTH = 200;
static uint8_t *pLocalBuffer = NULL;

static uint8_t uUartIndex = 0;

/// 初始化串口数量
void InitUartCount(uint8_t unMaxUartSize) {
  if (NULL == pUartInfoArray) {
    /// 此处程序刚开始，如果没有空间说明芯片不合适
    /// 故没有进行判断指针为空的问题
    pUartInfoArray = RequestSpace(sizeof(*pUartInfoArray) * unMaxUartSize);
    pLocalBuffer = RequestSpace(UART_BUFFER_LENGTH);
  }
}
/**
 * 创建一个接收串口的缓冲区
 */
uint8_t AddUart(UART_HandleTypeDef *pHUart, ReceiveUartCallback pCallback) {
  if (NULL == pUartInfoArray) {
    return (0);
  }

  Uart_Info *pUartInfo = RequestSpace(sizeof(*pUartInfo));

  if (NULL == pUartInfo) {
    return (0);
  } else {
#ifdef USE_FREERTOS
    pUartInfo->hQueueId = osMessageQueueNew(10, sizeof(UartQueueInfo), NULL);
#endif
    /// 绑定
    pUartInfo->pHUart = pHUart;
    pUartInfo->pCallback = pCallback;

    pUartInfo->pBuffer = RequestSpace(UART_BUFFER_LENGTH);
    pUartInfo->pReceive = pUartInfo->pBuffer;

    pUartInfoArray[uUartIndex] = pUartInfo;
  }

  return (++uUartIndex);
}

/// 获取串口句柄
UART_HandleTypeDef *GetUart(uint8_t uId) {
  if (uId < 1 || uId > uUartIndex) {
    return (NULL);
  } else {
    return (pUartInfoArray[uId - 1]->pHUart);
  }
}
/// 开始接收串口数据
void BeginReceiveUartInfo(uint8_t uId) {
  if (uId < 1 || uId > uUartIndex)
    return;
  Uart_Info *pUartInfo = pUartInfoArray[uId - 1];
  HAL_UARTEx_ReceiveToIdle_DMA(pUartInfo->pHUart, pUartInfo->pReceive,
                               UART_BUFFER_LENGTH);
}

/// 停止接收串口数据
void StopReceiveUartInfo(uint8_t uId) {
  if (uId < 1 || uId > uUartIndex)
    return;
  HAL_UART_DMAStop(pUartInfoArray[uId - 1]->pHUart);
}

/// DMA满了或者数据传输完毕的回调函数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *pHUart, uint16_t nSize) {
  static UartQueueInfo LOCAL_QUEUE_INFO;
  for (uint8_t index = 0; index < uUartIndex; ++index) {
    Uart_Info *pUartInfo = pUartInfoArray[index];
    if (pHUart == pUartInfo->pHUart) {
      LOCAL_QUEUE_INFO.pBuffer = pUartInfo->pReceive;
      LOCAL_QUEUE_INFO.nLength = nSize; // 获取DMA中传输的数据个数
#ifdef USE_FREERTOS
      if (osOK ==
          osMessageQueuePut(pUartInfo->hQueueId, &LOCAL_QUEUE_INFO, 0, 0))
#endif
      {
        pUartInfo->stAllIOInfo.unReciveCount += nSize;
      }

      pUartInfo->pReceive += nSize; // 指针向后移动
      if (pUartInfo->pReceive - pUartInfo->pBuffer >= UART_BUFFER_LENGTH) {
        pUartInfo->pReceive -= UART_BUFFER_LENGTH;
      }
      HAL_UARTEx_ReceiveToIdle_DMA(pUartInfo->pHUart, pUartInfo->pReceive,
                                   UART_BUFFER_LENGTH);
    }
  }
}

/// 定时处理数据
void ProcessUart(void) {
#ifdef USE_FREERTOS
  static UartQueueInfo LOCAL_QUEUE_INFO;
  for (uint8_t index = 0; index < uUartIndex; ++index) {
    Uart_Info *pUartInfo = pUartInfoArray[index];
    if (NULL != pUartInfo->pHUart) {
      while (osMessageQueueGetCount(pUartInfo->hQueueId) > 0 &&
             osOK == osMessageQueueGet(pUartInfo->hQueueId, &LOCAL_QUEUE_INFO,
                                       0, 0)) {
        /// 如果长度小于整个缓冲区得长度
        if ((LOCAL_QUEUE_INFO.pBuffer - pUartInfo->pBuffer) +
                LOCAL_QUEUE_INFO.nLength <=
            UART_BUFFER_LENGTH) {
          memcpy(pLocalBuffer, LOCAL_QUEUE_INFO.pBuffer,
                 LOCAL_QUEUE_INFO.nLength);
        } else {
          uint16_t nSubSize = (LOCAL_QUEUE_INFO.pBuffer - pUartInfo->pBuffer) +
                              LOCAL_QUEUE_INFO.nLength - UART_BUFFER_LENGTH;
          uint16_t nPreSize = LOCAL_QUEUE_INFO.nLength - nSubSize;
          if (nPreSize > 0)
            memcpy(pLocalBuffer, LOCAL_QUEUE_INFO.pBuffer, nPreSize);
          if (nSubSize > 0)
            memcpy(pLocalBuffer + nPreSize, pUartInfo->pBuffer, nSubSize);
        }

        pUartInfo->pCallback(pUartInfo->pHUart, pLocalBuffer,
                             LOCAL_QUEUE_INFO.nLength);
        pUartInfo->stAllIOInfo.unDealCount += LOCAL_QUEUE_INFO.nLength;
      }
    }
  }
#endif
}

/// 获取串口接收数据信息
const IOInfo *GetUartIOInfo(uint8_t uId) {
  if (uId >= 1 && uId <= uUartIndex) {
    Uart_Info *pUartInfo = pUartInfoArray[uId - 1];
    if (NULL != pUartInfo) {
      return (&pUartInfo->stAllIOInfo);
    }
  }
  return (NULL);
}

/// 更新串口发送数据
void UpdateUartSendInfo(UART_HandleTypeDef *pHUart, uint16_t unLength) {
  for (uint8_t index = 0; index < uUartIndex; ++index) {
    Uart_Info *pUartInfo = pUartInfoArray[index];
    if (pHUart == pUartInfo->pHUart) {
      pUartInfo->stAllIOInfo.unSendCount += unLength;
    }
  }
}

/// 获取管理里面的串口数量
uint8_t GetUartCount(void) { return (uUartIndex); }
