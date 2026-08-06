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
#ifdef USE_FREERTOS
  osMessageQueueId_t hQueueId;   /// 串口获取消息队列
#endif
  IOInfo stAllIOInfo;            /// 串口收发数据统计
} Uart_Info;

static Uart_Info **pUartInfoArray = NULL;
static uint8_t uUartIndex = 0;
static uint8_t uUartCapacity = 0;

#define UART_RECEIVE_QUEUE_DEPTH 16U

_Static_assert((sizeof(UartQueueInfo) % sizeof(uint32_t)) == 0U,
               "UART queue item must be 32-bit aligned");

static HAL_StatusTypeDef StartReceive(Uart_Info *pUartInfo) {
  HAL_StatusTypeDef status;

  if ((pUartInfo == NULL) || (pUartInfo->pHUart == NULL) ||
      (pUartInfo->pBuffer == NULL)) {
    return HAL_ERROR;
  }
  if (pUartInfo->pHUart->RxState == HAL_UART_STATE_BUSY_RX) {
    return HAL_OK;
  }
  if (pUartInfo->pHUart->RxState != HAL_UART_STATE_READY) {
    return HAL_BUSY;
  }

  __HAL_UART_CLEAR_OREFLAG(pUartInfo->pHUart);
  pUartInfo->pHUart->ErrorCode = HAL_UART_ERROR_NONE;
  status = HAL_UARTEx_ReceiveToIdle_DMA(pUartInfo->pHUart,
                                        pUartInfo->pBuffer,
                                        UART_RECEIVE_BUFFER_LENGTH);
  if ((status == HAL_OK) && (pUartInfo->pHUart->hdmarx != NULL)) {
    __HAL_DMA_DISABLE_IT(pUartInfo->pHUart->hdmarx, DMA_IT_HT);
  }
  return status;
}

#ifdef USE_FREERTOS
/** Keep the newest frame when the queue is full (boot URCs must not drop ATI). */
static osStatus_t UartQueuePutLatest(osMessageQueueId_t queue,
                                     const UartQueueInfo *item)
{
  osStatus_t status = osMessageQueuePut(queue, item, 0, 0);
  if (status == osOK) {
    return osOK;
  }
  {
    UartQueueInfo discarded;
    (void)osMessageQueueGet(queue, &discarded, NULL, 0);
  }
  return osMessageQueuePut(queue, item, 0, 0);
}
#endif

/// 初始化串口数量
void InitUartCount(uint8_t unMaxUartSize) {
  if (NULL == pUartInfoArray) {
    /// 此处程序刚开始，如果没有空间说明芯片不合适
    /// 故没有进行判断指针为空的问题
    pUartInfoArray = RequestSpace(sizeof(*pUartInfoArray) * unMaxUartSize);
    if (pUartInfoArray != NULL) {
      uUartCapacity = unMaxUartSize;
    }
  }
}
/**
 * 创建一个接收串口的缓冲区
 */
uint8_t AddUart(UART_HandleTypeDef *pHUart, ReceiveUartCallback pCallback) {
  if ((NULL == pUartInfoArray) || (NULL == pHUart) ||
      (NULL == pCallback) || (uUartIndex >= uUartCapacity)) {
    return (0);
  }
  Uart_Info *pUartInfo = RequestSpace(sizeof(*pUartInfo));
  if (NULL == pUartInfo) {
    return (0);
  } else {
#ifdef USE_FREERTOS
    pUartInfo->hQueueId =
        osMessageQueueNew(UART_RECEIVE_QUEUE_DEPTH, sizeof(UartQueueInfo), NULL);
    if (pUartInfo->hQueueId == NULL) {
      RecycleSpace(pUartInfo);
      return (0);
    }
#endif
    /// 绑定
    pUartInfo->pHUart = pHUart;
    pUartInfo->pCallback = pCallback;

    pUartInfo->pBuffer = RequestSpace(UART_RECEIVE_BUFFER_LENGTH);
    if (NULL == pUartInfo->pBuffer) {
#ifdef USE_FREERTOS
      (void)osMessageQueueDelete(pUartInfo->hQueueId);
#endif
      RecycleSpace(pUartInfo);
      return (0);
    }

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
HAL_StatusTypeDef BeginReceiveUartInfo(uint8_t uId) {
  if (uId < 1 || uId > uUartIndex)
    return HAL_ERROR;
  return StartReceive(pUartInfoArray[uId - 1]);
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
      if (nSize > UART_RECEIVE_BUFFER_LENGTH) {
        nSize = UART_RECEIVE_BUFFER_LENGTH;
      }
      memcpy(LOCAL_QUEUE_INFO.buffer, pUartInfo->pBuffer, nSize);
      LOCAL_QUEUE_INFO.nLength = nSize; // 获取DMA中传输的数据个数
#ifdef USE_FREERTOS
      if (osOK == UartQueuePutLatest(pUartInfo->hQueueId, &LOCAL_QUEUE_INFO))
#endif
      {
        pUartInfo->stAllIOInfo.unReciveCount += nSize;
      }

      (void)StartReceive(pUartInfo);
      break;
    }
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *pHUart) {
  for (uint8_t index = 0; index < uUartIndex; ++index) {
    Uart_Info *pUartInfo = pUartInfoArray[index];
    if ((pUartInfo != NULL) && (pHUart == pUartInfo->pHUart)) {
      (void)StartReceive(pUartInfo);
      break;
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
        pUartInfo->pCallback(pUartInfo->pHUart, LOCAL_QUEUE_INFO.buffer,
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
