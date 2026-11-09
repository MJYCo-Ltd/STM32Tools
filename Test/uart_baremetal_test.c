/* Compile the actual UART driver without USE_FREERTOS. */
#include <UartReceive.h>
#include <Auxiliary.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static uint32_t mask;
static unsigned delivered, losses;
static void Receive(UART_HandleTypeDef *u, uint8_t byte);
static uint8_t inject;
uint32_t __get_PRIMASK(void) {
  return mask;
}
void __disable_irq(void) {
  mask = 1U;
}
void __set_PRIMASK(uint32_t value) {
  mask = value;
}
void *RequestSpace(size_t size) {
  return calloc(1U, size);
}
void RecycleSpace(void *p) {
  free(p);
}
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *u, uint8_t *p, uint16_t n) {
  assert(n == UART_RECEIVE_BUFFER_LENGTH);
  u->buffer = p;
  u->RxState = HAL_UART_STATE_BUSY_RX;
  return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_DMAStop(UART_HandleTypeDef *u) {
  u->RxState = HAL_UART_STATE_READY;
  return HAL_OK;
}
static void Consume(UART_HandleTypeDef *u, uint8_t *p, uint16_t n) {
  assert(!mask && u && n == 2U && p[0] == 'x');
  if (inject) {
    Receive(u, 'y');
    Receive(u, 'z');
    assert(p[0] == 'x' && p[1] == '\n');
  }
  ++delivered;
}
static void Loss(UART_HandleTypeDef *u, uint32_t count, void *ctx) {
  assert(!mask && u == ctx && count);
  ++losses;
}
static void Receive(UART_HandleTypeDef *u, uint8_t byte) {
  memcpy(u->buffer, "x\n", 2U);
  u->buffer[0] = byte;
  u->RxState = HAL_UART_STATE_READY;
  HAL_UARTEx_RxEventCallback(u, 2U);
}
int main(void) {
  static UART_HandleTypeDef uart, dedicated;
  InitUartCount(2U);
  assert(AddUart(&uart, Consume) == 1U);
  assert(UartReceive_AddDedicated(&dedicated, Consume) == 2U);
  UartReceive_SetLossCallback(Loss, &uart);
  assert(BeginReceiveUartInfo(1U) == HAL_OK);
  Receive(&uart, 'x');
  uart.RxState = HAL_UART_STATE_READY;
  HAL_UARTEx_RxEventCallback(&uart, 0U);
  assert(ProcessUartBudget(1U) == 1U && delivered == 1U);
  Receive(&uart, 'x');
  Receive(&uart, 'x'); /* the one-slot bare-metal inbox overflowed */
  assert(ProcessUartBudget(1U) == 1U && delivered == 1U && losses == 1U);
  Receive(&uart, 'x');
  inject = 1U;
  assert(ProcessUartBudget(1U) == 1U);
  assert(delivered == 2U);
  inject = 0U;
  assert(ProcessUartBudget(1U) == 1U && losses == 2U && delivered == 2U);
  Receive(&uart, 'x');
  StopReceiveUartInfo(1U);
  assert(!UartReceive_HasPending());
  assert(BeginReceiveUartInfo(1U) == HAL_OK);
  assert(ProcessUartBudget(1U) == 0U);
  assert(BeginReceiveUartInfo(2U) == HAL_OK);
  Receive(&dedicated, 'x');
  assert(!UartReceive_HasPending() && ProcessUartBudget(1U) == 0U);
  assert(UartReceive_ProcessPort(2U, 7U) == UART_RECEIVE_DATA && delivered == 3U);
  assert(UartReceive_ProcessPort(2U, 7U) == UART_RECEIVE_IDLE);
  return 0;
}
