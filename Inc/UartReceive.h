#ifndef STM32TOOLS_UART_RECEIVE_H
#define STM32TOOLS_UART_RECEIVE_H
#include "main.h"
#include "IOStatistics.h"

#define UART_RECEIVE_BUFFER_LENGTH 256U
#ifndef UART_RECEIVE_QUEUE_DEPTH
#define UART_RECEIVE_QUEUE_DEPTH 8U
#endif
#if UART_RECEIVE_QUEUE_DEPTH < 1U
#error "UART_RECEIVE_QUEUE_DEPTH must be positive"
#endif
#ifndef UART_RECEIVE_PROCESS_BUDGET
#define UART_RECEIVE_PROCESS_BUDGET 8U
#endif
#if UART_RECEIVE_PROCESS_BUDGET < 1U || UART_RECEIVE_PROCESS_BUDGET > 65535U
#error "UART_RECEIVE_PROCESS_BUDGET must be in 1..65535"
#endif

typedef struct {
  uint8_t buffer[UART_RECEIVE_BUFFER_LENGTH];
  uint16_t nLength, reserved;
} UartQueueInfo;
typedef void (*ReceiveUartCallback)(UART_HandleTypeDef *, uint8_t *, uint16_t);
typedef void (*UartReceiveLossCallback)(UART_HandleTypeDef *, uint32_t dropped_chunks,
                                        void *context);

void InitUartCount(uint8_t maximum);
uint8_t AddUart(UART_HandleTypeDef *uart, ReceiveUartCallback callback);
UART_HandleTypeDef *GetUart(uint8_t id);
uint8_t GetUartCount(void);
const IOInfo *GetUartIOInfo(uint8_t id);
void UpdateUartSendInfo(UART_HandleTypeDef *uart, uint16_t length);
HAL_StatusTypeDef BeginReceiveUartInfo(uint8_t id);
void StopReceiveUartInfo(uint8_t id);
/** Single task consumes RX. Round-robin bounded work; callbacks must not block
 * indefinitely. No parsing, application callback or allocation runs in ISR.
 * Normal-mode ReceiveToIdle DMA is required (not circular DMA).
 */
uint16_t ProcessUartBudget(uint16_t max_chunks);
void ProcessUart(void); /* at most UART_RECEIVE_PROCESS_BUDGET chunks globally */
uint8_t UartReceive_HasPending(void);
/** Configured at startup. Called in consumer-task context BEFORE any post-gap
 * data is delivered. All queued pre-gap chunks are discarded on a loss. */
void UartReceive_SetLossCallback(UartReceiveLossCallback callback, void *context);
uint32_t UartReceive_DroppedChunks(uint8_t id);
#endif
