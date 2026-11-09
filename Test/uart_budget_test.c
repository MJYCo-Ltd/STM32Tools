#include <UartReceive.h>
#include <Auxiliary.h>
#include "cmsis_os.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t masked;
static unsigned in_isr, calls[4], loss_calls[4], inject, start_fail;
static unsigned alloc_fail, queue_fail, live_allocations;
static size_t requested_bytes, queue_bytes;
static void (*on_wait)(void);
static uint32_t wait_ticks;
static uint8_t *finished_dma;
static DMA_HandleTypeDef dma;
static UART_HandleTypeDef a = {.id = 1, .hdmarx = &dma};
static UART_HandleTypeDef b = {.id = 2, .hdmarx = &dma};
static UART_HandleTypeDef c = {.id = 3, .hdmarx = &dma};
typedef struct {
  unsigned cap, size, head, count;
  uint8_t *data;
} Queue;
uint32_t __get_PRIMASK(void) {
  return masked;
}
void __disable_irq(void) {
  masked = 1;
}
void __set_PRIMASK(uint32_t p) {
  masked = p;
}
void *RequestSpace(size_t n) {
  assert(!in_isr);
  if (alloc_fail != 0U && --alloc_fail == 0U)
    return NULL;
  requested_bytes += n;
  ++live_allocations;
  return calloc(1, n);
}
void RecycleSpace(void *p) {
  assert(!in_isr);
  if (p)
    --live_allocations;
  free(p);
}
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *u, uint8_t *p, uint16_t n) {
  assert(n == 256);
  if (start_fail) {
    --start_fail;
    return HAL_ERROR;
  }
  u->buffer = p;
  u->RxState = 1;
  return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_DMAStop(UART_HandleTypeDef *u) {
  u->RxState = 0;
  return HAL_OK;
}
osMessageQueueId_t osMessageQueueNew(uint32_t cap, uint32_t size, const void *attr) {
  (void)attr;
  assert(!in_isr && cap == 8U && size == 260U);
  if (queue_fail) {
    --queue_fail;
    return NULL;
  }
  Queue *q = calloc(1, sizeof(*q));
  assert(q);
  q->data = calloc(cap, size);
  assert(q->data);
  q->cap = cap;
  q->size = size;
  queue_bytes += cap * size;
  return q;
}
osStatus_t osMessageQueuePut(osMessageQueueId_t handle, const void *data, uint8_t prio,
                             uint32_t timeout) {
  (void)prio;
  assert(timeout == 0U);
  if (in_isr && ((const UartQueueInfo *)data)->nLength != 0U)
    assert(data == finished_dma);
  Queue *q = handle;
  if (q->count == q->cap)
    return osErrorResource;
  memcpy(q->data + ((q->head + q->count) % q->cap) * q->size, data, q->size);
  ++q->count;
  return osOK;
}
osStatus_t osMessageQueueGet(osMessageQueueId_t handle, void *data, uint8_t *prio,
                             uint32_t timeout) {
  (void)prio;
  assert(!masked && !in_isr);
  Queue *q = handle;
  if (!q->count && timeout != 0U) {
    wait_ticks = timeout;
    if (on_wait)
      on_wait();
  }
  if (!q->count)
    return osErrorResource;
  memcpy(data, q->data + q->head * q->size, q->size);
  q->head = (q->head + 1) % q->cap;
  --q->count;
  return osOK;
}
osStatus_t osMessageQueueDelete(osMessageQueueId_t handle) {
  Queue *q = handle;
  queue_bytes -= q->cap * q->size;
  free(q->data);
  free(q);
  return osOK;
}
uint32_t osMessageQueueGetCount(osMessageQueueId_t handle) {
  return ((Queue *)handle)->count;
}
static void Chunk(UART_HandleTypeDef *u, uint16_t size, uint8_t value) {
  memset(u->buffer, value, UART_RECEIVE_BUFFER_LENGTH);
  u->buffer[1] = '\n';
  u->RxState = 0;
  in_isr = 1;
  finished_dma = u->buffer;
  HAL_UARTEx_RxEventCallback(u, size);
  in_isr = 0;
}
static void Rx(UART_HandleTypeDef *u) {
  Chunk(u, 2U, 'x');
}
static void WaitForData(void) {
  Chunk(&c, UART_RECEIVE_BUFFER_LENGTH, 'x');
}
static void WaitForError(void) {
  c.RxState = HAL_UART_STATE_READY;
  in_isr = 1;
  HAL_UART_ErrorCallback(&c);
  in_isr = 0;
}
static void Delivered(UART_HandleTypeDef *u, uint8_t *p, uint16_t n) {
  assert(!in_isr && !masked && p[1] == '\n');
  assert(n == 2U || (n == UART_RECEIVE_BUFFER_LENGTH && p[255] == 'x'));
  ++calls[u->id];
  if (inject && u == &a)
    Rx(&a);
  if (inject && u == &c) {
    assert(p[0] == 'x');
    Chunk(&c, 2U, 'y');
    Chunk(&c, 2U, 'z');
    assert(p[0] == 'x' && p[255] == 'x');
  }
}
static void Lost(UART_HandleTypeDef *u, uint32_t count, void *ctx) {
  assert(!in_isr && !masked && count > 0 && ctx == &a);
  ++loss_calls[u->id];
}
int main(void) {
  alloc_fail = 1U;
  InitUartCount(3);
  assert(!AddUart(&a, Delivered) && live_allocations == 0U);
  InitUartCount(3);
  for (unsigned failure = 1U; failure <= 3U; ++failure) {
    alloc_fail = failure;
    assert(!AddUart(&a, Delivered));
    assert(GetUartCount() == 0U && live_allocations == 1U && queue_bytes == 0U);
  }
  queue_fail = 1U;
  assert(!AddUart(&a, Delivered));
  assert(GetUartCount() == 0U && live_allocations == 1U && queue_bytes == 0U);
  size_t before_alloc = requested_bytes;
  assert(AddUart(&a, Delivered) == 1);
  size_t per_port_bytes = requested_bytes - before_alloc;
  printf("UART allocations/port: %zu state + %zu DMA + %zu queue payload = %zu requested bytes (excludes allocator and queue control)\n",
         per_port_bytes - 2U * sizeof(UartQueueInfo), 2U * sizeof(UartQueueInfo),
         queue_bytes, per_port_bytes + queue_bytes);
  assert(AddUart(&b, Delivered) == 2);
  assert(UartReceive_AddDedicated(&c, Delivered) == 3);
  assert(!AddUart(&a, Delivered));
  assert(BeginReceiveUartInfo(1) == HAL_OK && BeginReceiveUartInfo(2) == HAL_OK);
  UartReceive_SetLossCallback(Lost, &a);
  Rx(&a);
  Rx(&b);
  inject = 1;
  assert(ProcessUartBudget(5) == 5);
  assert(calls[2] == 1 && calls[1] == 4);
  inject = 0;
  ProcessUart();
  assert(!UartReceive_HasPending());
  Rx(&a);
  Rx(&b);
  inject = 1;
  unsigned total_before = calls[1] + calls[2];
  unsigned second_before = calls[2];
  ProcessUart();
  assert(calls[1] + calls[2] - total_before == UART_RECEIVE_PROCESS_BUDGET);
  assert(calls[2] == second_before + 1U);
  inject = 0;
  ProcessUart();
  unsigned before = calls[1];
  for (unsigned i = 0; i < UART_RECEIVE_QUEUE_DEPTH + 3; ++i)
    Rx(&a);
  assert(UartReceive_DroppedChunks(1) > 0U);
  assert(!loss_calls[1]);
  ProcessUart();
  assert(loss_calls[1] == 1 && calls[1] == before);
  Rx(&a);
  ProcessUart();
  assert(calls[1] == before + 1U);
  HAL_UART_ErrorCallback(&a);
  ProcessUart();
  assert(loss_calls[1] == 2U);
  Rx(&a);
  StopReceiveUartInfo(1);
  before = calls[1];
  Rx(&a);
  ProcessUart();
  assert(calls[1] == before);
  assert(!UartReceive_HasPending());
  assert(BeginReceiveUartInfo(1) == HAL_OK);
  assert(ProcessUartBudget(1U) == 0U);
  Rx(&a);
  assert(ProcessUartBudget(1U) == 1U && calls[1] == before + 1U);

  assert(BeginReceiveUartInfo(3) == HAL_OK);
  Rx(&c);
  assert(!UartReceive_HasPending() && ProcessUartBudget(8U) == 0U);
  assert(UartReceive_ProcessPort(1U, 20U) == UART_RECEIVE_IDLE);
  assert(UartReceive_ProcessPort(3U, 0U) == UART_RECEIVE_DATA);
  on_wait = WaitForData;
  inject = 1;
  assert(UartReceive_ProcessPort(3U, 37U) == UART_RECEIVE_DATA && wait_ticks == 37U);
  inject = 0;
  assert(UartReceive_ProcessPort(3U, 0U) == UART_RECEIVE_DATA);
  assert(UartReceive_ProcessPort(3U, 0U) == UART_RECEIVE_DATA);
  assert(calls[3] == 4U);
  on_wait = WaitForError;
  start_fail = 1U;
  assert(UartReceive_ProcessPort(3U, 29U) == UART_RECEIVE_LOSS && wait_ticks == 29U);
  assert(loss_calls[3] == 1U && !UartReceive_HasPending());
  on_wait = NULL;
  assert(UartReceive_ProcessPort(3U, 5U) == UART_RECEIVE_IDLE && wait_ticks == 5U);
  assert(c.RxState == HAL_UART_STATE_BUSY_RX);
  Chunk(&c, UART_RECEIVE_BUFFER_LENGTH + 1U, '!');
  assert(UartReceive_ProcessPort(3U, 0U) == UART_RECEIVE_LOSS && loss_calls[3] == 2U);
  start_fail = 1U;
  Rx(&c);
  assert(UartReceive_ProcessPort(3U, 0U) == UART_RECEIVE_LOSS && loss_calls[3] == 3U);
  assert(c.RxState == HAL_UART_STATE_BUSY_RX && calls[3] == 4U);
  Rx(&c);
  StopReceiveUartInfo(3U);
  assert(UartReceive_ProcessPort(3U, 1U) == UART_RECEIVE_IDLE);
  assert(BeginReceiveUartInfo(3U) == HAL_OK);
  assert(UartReceive_ProcessPort(3U, 0U) == UART_RECEIVE_IDLE);
  Rx(&c);
  assert(UartReceive_ProcessPort(3U, 0U) == UART_RECEIVE_DATA);
  WaitForError();
  StopReceiveUartInfo(3U);
  assert(loss_calls[3] == 4U && BeginReceiveUartInfo(3U) == HAL_OK);
  assert(UartReceive_ProcessPort(3U, 0U) == UART_RECEIVE_IDLE);
  puts("UART bounded work/fairness/loss notification/stop: passed");
  return 0;
}
