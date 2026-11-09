/*

 * UartReceive.c

 *

 *  Created on: Apr 10, 2024

 *      Author: yty

 */

#include "UartReceive.h"
#include "Auxiliary.h"
#include "Base.h"

typedef struct {
  UART_HandleTypeDef *uart;
  ReceiveUartCallback callback;
  UartQueueInfo *buffer, *alternate;
  UartQueueInfo process_frame;
  volatile uint8_t restart_pending, enabled, loss_pending;
  uint8_t dedicated;
  volatile uint32_t dropped;
#ifdef USE_FREERTOS
  osMessageQueueId_t queue;
#else
  volatile uint8_t frame_pending;
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
#ifdef USE_FREERTOS
  uint8_t already_pending = p->loss_pending;
#endif
  p->loss_pending = 1U;
  if (p->dropped != UINT32_MAX)
    ++p->dropped;
#ifdef USE_FREERTOS
  if (p->dedicated && !already_pending) {
    /* Wake a blocked owner even when the error produced no data. A full
     * queue already wakes it. Take discards this marker with the old data. */
    static const UartQueueInfo wake = {0};
    (void)osMessageQueuePut(p->queue, &wake, 0U, 0U);
  }
#endif
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
      HAL_UARTEx_ReceiveToIdle_DMA(p->uart, p->buffer->buffer, UART_RECEIVE_BUFFER_LENGTH);
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
static uint8_t Register(UART_HandleTypeDef *uart, ReceiveUartCallback callback,
                        uint8_t dedicated) {
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
  p->buffer = RequestSpace(sizeof(*p->buffer));
  p->alternate = RequestSpace(sizeof(*p->alternate));
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
  p->dedicated = dedicated;
  /* Earlier ports can already receive interrupts during board startup. */
  uint32_t lock = Lock();
  uint8_t id = s_count + 1U;
  s_ports[s_count] = p;
  s_count = id;
  Unlock(lock);
  return id;
}
uint8_t AddUart(UART_HandleTypeDef *uart, ReceiveUartCallback callback) {
  return Register(uart, callback, 0U);
}
uint8_t UartReceive_AddDedicated(UART_HandleTypeDef *uart, ReceiveUartCallback callback) {
  return Register(uart, callback, 1U);
}
UART_HandleTypeDef *GetUart(uint8_t id) {
  return id != 0U && id <= s_count ? s_ports[id - 1U]->uart : NULL;
}
HAL_StatusTypeDef BeginReceiveUartInfo(uint8_t id) {
  if (id == 0U || id > s_count)
    return HAL_ERROR;
  UartInfo *p = s_ports[id - 1U];
  uint32_t lock = Lock();
  p->enabled = 1U;
  HAL_StatusTypeDef status = Start(p);
  Unlock(lock);
  return status;
}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *uart, uint16_t size) {
  UartInfo *p = Find(uart);
  if (p == NULL || !p->enabled)
    return;
  if (size == 0U) {
    if (Start(p) != HAL_OK)
      Lost(p);
    return;
  }
  UartQueueInfo *ready = p->buffer;
  p->buffer = p->alternate;
  p->alternate = ready;
  p->io.unReciveCount += size <= UART_RECEIVE_BUFFER_LENGTH ?
      size : UART_RECEIVE_BUFFER_LENGTH;
  if (Start(p) != HAL_OK || size > UART_RECEIVE_BUFFER_LENGTH || p->loss_pending) {
    Lost(p);
    return;
  }
  ready->nLength = size;
  ready->reserved = 0U;
#ifdef USE_FREERTOS
  if (osMessageQueuePut(p->queue, ready, 0U, 0U) != osOK)
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
static void Discard(UartInfo *p) {
#ifdef USE_FREERTOS
  /* RX is stopped or dropping new data. Draining is finite. */
  while (osMessageQueueGet(p->queue, &p->process_frame, NULL, 0U) == osOK) {
  }
#else
  p->frame_pending = 0U;
#endif
}
static UartReceiveResult ReportLoss(UartInfo *p) {
  Discard(p);
  uint32_t lock = Lock(), dropped = p->dropped;
  p->loss_pending = 0U;
  Unlock(lock);
  if (s_loss_callback)
    s_loss_callback(p->uart, dropped, s_loss_context);
  return UART_RECEIVE_LOSS;
}
void StopReceiveUartInfo(uint8_t id) {
  if (id == 0U || id > s_count)
    return;
  UartInfo *p = s_ports[id - 1U];
  uint32_t lock = Lock();
  p->enabled = 0U;
  p->restart_pending = 0U;
  Unlock(lock);
  (void)HAL_UART_DMAStop(p->uart);
  if (p->loss_pending)
    (void)ReportLoss(p);
  else
    Discard(p);
}
static UartReceiveResult Take(UartInfo *p, uint32_t timeout_ticks) {
  if (!p->enabled)
    return UART_RECEIVE_IDLE;
  if (p->restart_pending) {
    uint32_t lock = Lock();
    (void)Start(p);
    Unlock(lock);
  }
  if (p->loss_pending)
    return ReportLoss(p);
  if (p->restart_pending)
    return UART_RECEIVE_IDLE;
  uint8_t have = 0U;
#ifdef USE_FREERTOS
  have = osMessageQueueGet(p->queue, &p->process_frame, NULL, timeout_ticks) == osOK;
#else
  (void)timeout_ticks;
  uint32_t lock = Lock();
  if (p->frame_pending && !p->loss_pending) {
    p->process_frame = *p->alternate;
    p->frame_pending = 0U;
    have = 1U;
  }
  Unlock(lock);
#endif
  if (p->loss_pending)
    return ReportLoss(p);
  if (have) {
    p->callback(p->uart, p->process_frame.buffer, p->process_frame.nLength);
    p->io.unDealCount += p->process_frame.nLength;
  }
  return have ? UART_RECEIVE_DATA : UART_RECEIVE_IDLE;
}
UartReceiveResult UartReceive_ProcessPort(uint8_t id, uint32_t timeout_ticks) {
  if (id == 0U || id > s_count || !s_ports[id - 1U]->dedicated)
    return UART_RECEIVE_IDLE;
  return Take(s_ports[id - 1U], timeout_ticks);
}
uint16_t ProcessUartBudget(uint16_t maximum) {
  uint16_t work = 0U;
  uint8_t idle = 0U;
  while (s_count != 0U && work < maximum && idle < s_count) {
    UartInfo *p = s_ports[s_next];
    s_next = (uint8_t)((s_next + 1U) % s_count);
    if (!p->dedicated && Take(p, 0U) != UART_RECEIVE_IDLE) {
      ++work;
      idle = 0U;
    } else
      ++idle;
  }
  return work;
}
void ProcessUart(void) {
  (void)ProcessUartBudget(UART_RECEIVE_PROCESS_BUDGET);
}
uint8_t UartReceive_HasPending(void) {
  for (uint8_t i = 0; i < s_count; ++i) {
    UartInfo *p = s_ports[i];
    if (p->dedicated || !p->enabled)
      continue;
    if (p->loss_pending || p->restart_pending)
      return 1U;
#ifdef USE_FREERTOS
    if (osMessageQueueGetCount(p->queue))
      return 1U;
#else
    if (p->frame_pending)
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
