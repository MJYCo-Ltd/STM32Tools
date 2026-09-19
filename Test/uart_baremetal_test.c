/* Compile the actual UART driver without USE_FREERTOS. */
#include <UartReceive.h>
#include <Auxiliary.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static uint32_t mask;
static unsigned delivered, losses;
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
  ++delivered;
}
static void Loss(UART_HandleTypeDef *u, uint32_t count, void *ctx) {
  assert(!mask && u == ctx && count);
  ++losses;
}
static void Receive(UART_HandleTypeDef *u) {
  memcpy(u->buffer, "x\n", 2U);
  u->RxState = HAL_UART_STATE_READY;
  HAL_UARTEx_RxEventCallback(u, 2U);
}
int main(void) {
  static UART_HandleTypeDef uart;
  InitUartCount(1U);
  assert(AddUart(&uart, Consume) == 1U);
  UartReceive_SetLossCallback(Loss, &uart);
  assert(BeginReceiveUartInfo(1U) == HAL_OK);
  Receive(&uart);
  assert(ProcessUartBudget(1U) == 1U && delivered == 1U);
  Receive(&uart);
  Receive(&uart); /* the one-slot bare-metal inbox overflowed */
  assert(ProcessUartBudget(1U) == 1U && delivered == 1U && losses == 1U);
  Receive(&uart);
  ProcessUart();
  assert(delivered == 2U);
  StopReceiveUartInfo(1U);
  assert(!UartReceive_HasPending());
  return 0;
}
