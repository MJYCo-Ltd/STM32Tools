/*

 * UartReceive.c

 *

 *  Created on: Apr 10, 2024

 *      Author: yty

 */

#include "UartReceive.h"
#include "Auxiliary.h"
#include "Base.h"
#include <string.h>

typedef struct {
  UART_HandleTypeDef *uart;
  ReceiveUartCallback callback;
  uint8_t *buffer, *alternate;
  UartQueueInfo staging, process_frame;
  volatile uint8_t restart_pending, frame_pending, enabled, loss_pending;
  volatile uint32_t dropped;
#ifdef USE_FREERTOS
  osMessageQueueId_t queue;
#endif
  IOInfo io;
} UartInfo;
static UartInfo **s_ports;
static uint8_t s_count, s_capacity, s_next;
static UartReceiveLossCallback s_loss_callback;
static void *s_loss_context;
_Static_assert(sizeof(UartQueueInfo) % sizeof(uint32_t) == 0U, "UART queue alignment");
static uint32_t Lock(void) {
  uint32_t p = __get_PRIMASK();
  __disable_irq();
  return p;
}
static void Unlock(uint32_t p) {
  __set_PRIMASK(p);
}
static UartInfo *Find(const UART_HandleTypeDef *uart) {
  if (uart == NULL)
    return NULL;
  for (uint8_t i = 0; i < s_count; ++i)
    if (s_ports[i]->uart == uart)
      return s_ports[i];
  return NULL;
}
static void Lost(UartInfo *p) {
  p->loss_pending = 1U;
  if (p->dropped != UINT32_MAX)
    ++p->dropped;
}
static HAL_StatusTypeDef Start(UartInfo *p) {
  if (p == NULL || p->uart == NULL || p->buffer == NULL || !p->enabled)
    return HAL_ERROR;
  if (p->uart->RxState == HAL_UART_STATE_BUSY_RX) {
    p->restart_pending = 0U;
    return HAL_OK;
  }
  if (p->uart->RxState != HAL_UART_STATE_READY) {
    p->restart_pending = 1U;
    return HAL_BUSY;
  }
  __HAL_UART_CLEAR_OREFLAG(p->uart);
  p->uart->ErrorCode = HAL_UART_ERROR_NONE;
  HAL_StatusTypeDef status =
      HAL_UARTEx_ReceiveToIdle_DMA(p->uart, p->buffer, UART_RECEIVE_BUFFER_LENGTH);
  p->restart_pending = status != HAL_OK;
  if (status == HAL_OK && p->uart->hdmarx != NULL)
    __HAL_DMA_DISABLE_IT(p->uart->hdmarx, DMA_IT_HT);
  return status;
}
void InitUartCount(uint8_t maximum) {
  if (s_ports == NULL && maximum != 0U) {
    s_ports = RequestSpace(sizeof(*s_ports) * maximum);
    if (s_ports)
      s_capacity = maximum;
  }
}
uint8_t AddUart(UART_HandleTypeDef *uart, ReceiveUartCallback callback) {
  if (s_ports == NULL || uart == NULL || callback == NULL || s_count >= s_capacity ||
      Find(uart) != NULL)
    return 0U;
  UartInfo *p = RequestSpace(sizeof(*p));
  if (p == NULL)
    return 0U;
#ifdef USE_FREERTOS
  p->queue = osMessageQueueNew(UART_RECEIVE_QUEUE_DEPTH, sizeof(UartQueueInfo), NULL);
  if (p->queue == NULL) {
    RecycleSpace(p);
    return 0U;
  }
#endif
  p->buffer = RequestSpace(UART_RECEIVE_BUFFER_LENGTH);
  p->alternate = RequestSpace(UART_RECEIVE_BUFFER_LENGTH);
  if (p->buffer == NULL || p->alternate == NULL) {
    RecycleSpace(p->buffer);
    RecycleSpace(p->alternate);
#ifdef USE_FREERTOS
    (void)osMessageQueueDelete(p->queue);
#endif
    RecycleSpace(p);
    return 0U;
  }
  p->uart = uart;
  p->callback = callback;
  s_ports[s_count++] = p;
  return s_count;
}
UART_HandleTypeDef *GetUart(uint8_t id) {
  return id != 0U && id <= s_count ? s_ports[id - 1U]->uart : NULL;
}
HAL_StatusTypeDef BeginReceiveUartInfo(uint8_t id) {
  if (id == 0U || id > s_count)
    return HAL_ERROR;
  s_ports[id - 1U]->enabled = 1U;
  return Start(s_ports[id - 1U]);
}
void StopReceiveUartInfo(uint8_t id) {
  if (id == 0U || id > s_count)
    return;
  UartInfo *p = s_ports[id - 1U];
  p->enabled = 0U;
  p->restart_pending = 0U;
  (void)HAL_UART_DMAStop(p->uart);
}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *uart, uint16_t size) {
  UartInfo *p = Find(uart);
  if (p == NULL || !p->enabled)
    return;
  if (size > UART_RECEIVE_BUFFER_LENGTH) {
    Lost(p);
    size = UART_RECEIVE_BUFFER_LENGTH;
  }
  uint8_t *ready = p->buffer;
  p->buffer = p->alternate;
  p->alternate = ready;
  (void)Start(p);
  p->io.unReciveCount += size;
  if (p->loss_pending) {
    Lost(p);
    return;
  }
  if (size == 0U)
    return;
  memcpy(p->staging.buffer, ready, size);
  p->staging.nLength = size;
  p->staging.reserved = 0U;
#ifdef USE_FREERTOS
  if (osMessageQueuePut(p->queue, &p->staging, 0U, 0U) != osOK)
    Lost(p);
#else
  if (p->frame_pending)
    Lost(p);
  else
    p->frame_pending = 1U;
#endif
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart) {
  UartInfo *p = Find(uart);
  if (p == NULL || !p->enabled)
    return;
  Lost(p);
  (void)Start(p);
}
static uint8_t Take(UartInfo *p) {
  uint8_t lost = p->loss_pending, have = 0U;
  if (lost) {
#ifdef USE_FREERTOS
    /* ISR drops new chunks while loss_pending is set. Draining is finite;
     * never invoke RTOS queue APIs with globally masked thread interrupts. */
    while (osMessageQueueGet(p->queue, &p->process_frame, NULL, 0U) == osOK) {
    }
#endif
    uint32_t lock = Lock(), dropped = p->dropped;
    p->frame_pending = 0U;
    p->loss_pending = 0U;
    Unlock(lock);
    if (s_loss_callback)
      s_loss_callback(p->uart, dropped, s_loss_context);
    return 1U;
  }
#ifdef USE_FREERTOS
  have = osMessageQueueGet(p->queue, &p->process_frame, NULL, 0U) == osOK;
#else
  uint32_t lock = Lock();
  if (p->frame_pending && !p->loss_pending) {
    p->process_frame = p->staging;
    p->frame_pending = 0U;
    have = 1U;
  }
  Unlock(lock);
#endif
  if (have) {
    p->callback(p->uart, p->process_frame.buffer, p->process_frame.nLength);
    p->io.unDealCount += p->process_frame.nLength;
  }
  return have;
}
uint16_t ProcessUartBudget(uint16_t maximum) {
  uint16_t work = 0U;
  uint8_t idle = 0U;
  while (s_count != 0U && work < maximum && idle < s_count) {
    UartInfo *p = s_ports[s_next];
    s_next = (uint8_t)((s_next + 1U) % s_count);
    if (p->restart_pending && p->enabled)
      (void)Start(p);
    if (Take(p)) {
      ++work;
      idle = 0U;
    } else
      ++idle;
  }
  return work;
}
void ProcessUart(void) {
  (void)ProcessUartBudget(8U);
}
uint8_t UartReceive_HasPending(void) {
  for (uint8_t i = 0; i < s_count; ++i) {
    UartInfo *p = s_ports[i];
    if (p->loss_pending || p->restart_pending || p->frame_pending)
      return 1U;
#ifdef USE_FREERTOS
    if (osMessageQueueGetCount(p->queue))
      return 1U;
#endif
  }
  return 0U;
}
void UartReceive_SetLossCallback(UartReceiveLossCallback callback, void *context) {
  s_loss_callback = callback;
  s_loss_context = context;
}
uint32_t UartReceive_DroppedChunks(uint8_t id) {
  return id != 0U && id <= s_count ? s_ports[id - 1U]->dropped : 0U;
}
const IOInfo *GetUartIOInfo(uint8_t id) {
  return id != 0U && id <= s_count ? &s_ports[id - 1U]->io : NULL;
}
void UpdateUartSendInfo(UART_HandleTypeDef *uart, uint16_t length) {
  UartInfo *p = Find(uart);
  if (p)
    p->io.unSendCount += length;
}
uint8_t GetUartCount(void) {
  return s_count;
}
