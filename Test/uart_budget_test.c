#include <UartReceive.h>
#include <Auxiliary.h>
#include "cmsis_os.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t masked;
static unsigned in_isr, calls[3], loss_calls, inject;
static DMA_HandleTypeDef dma;
static UART_HandleTypeDef a = {.id = 1, .hdmarx = &dma}, b = {.id = 2, .hdmarx = &dma};
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
  return calloc(1, n);
}
void RecycleSpace(void *p) {
  free(p);
}
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *u, uint8_t *p, uint16_t n) {
  assert(n == 256);
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
  Queue *q = calloc(1, sizeof(*q));
  assert(q);
  q->data = calloc(cap, size);
  assert(q->data);
  q->cap = cap;
  q->size = size;
  return q;
}
osStatus_t osMessageQueuePut(osMessageQueueId_t handle, const void *data, uint8_t prio,
                             uint32_t timeout) {
  (void)prio;
  assert(timeout == 0U);
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
  assert(timeout == 0U && !masked);
  Queue *q = handle;
  if (!q->count)
    return osErrorResource;
  memcpy(data, q->data + q->head * q->size, q->size);
  q->head = (q->head + 1) % q->cap;
  --q->count;
  return osOK;
}
osStatus_t osMessageQueueDelete(osMessageQueueId_t handle) {
  Queue *q = handle;
  free(q->data);
  free(q);
  return osOK;
}
uint32_t osMessageQueueGetCount(osMessageQueueId_t handle) {
  return ((Queue *)handle)->count;
}
static void Rx(UART_HandleTypeDef *u) {
  memcpy(u->buffer, "x\n", 2);
  u->RxState = 0;
  in_isr = 1;
  HAL_UARTEx_RxEventCallback(u, 2);
  in_isr = 0;
}
static void Delivered(UART_HandleTypeDef *u, uint8_t *p, uint16_t n) {
  assert(!in_isr && !masked && n == 2 && p[0] == 'x');
  ++calls[u->id];
  if (inject && u == &a)
    Rx(&a);
}
static void Lost(UART_HandleTypeDef *u, uint32_t count, void *ctx) {
  assert(!in_isr && !masked && u == &a && count > 0 && ctx == &a);
  ++loss_calls;
}
int main(void) {
  InitUartCount(2);
  assert(AddUart(&a, Delivered) == 1);
  assert(AddUart(&b, Delivered) == 2);
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
  assert(!loss_calls);
  ProcessUart();
  assert(loss_calls == 1 && calls[1] == before);
  Rx(&a);
  ProcessUart();
  assert(calls[1] == before + 1U);
  HAL_UART_ErrorCallback(&a);
  ProcessUart();
  assert(loss_calls == 2U);
  StopReceiveUartInfo(1);
  before = calls[1];
  Rx(&a);
  ProcessUart();
  assert(calls[1] == before);
  assert(!UartReceive_HasPending());
  puts("UART bounded work/fairness/loss notification/stop: passed");
  return 0;
}
