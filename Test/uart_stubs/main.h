#ifndef UART_TEST_MAIN_H
#define UART_TEST_MAIN_H
#include <stdint.h>
#include <stddef.h>
typedef enum { HAL_OK=0, HAL_ERROR=1, HAL_BUSY=2 } HAL_StatusTypeDef;
typedef struct { unsigned unused; } DMA_HandleTypeDef;
typedef struct { uint32_t RxState, ErrorCode; DMA_HandleTypeDef *hdmarx; unsigned id; } UART_HandleTypeDef;
#define HAL_UART_STATE_READY 0U
#define HAL_UART_STATE_BUSY_RX 1U
#define HAL_UART_ERROR_NONE 0U
#define DMA_IT_HT 1U
#define __HAL_UART_CLEAR_OREFLAG(u) ((void)(u))
#define __HAL_DMA_DISABLE_IT(d,i) ((void)(d),(void)(i))
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *,uint8_t *,uint16_t);
void HAL_UART_DMAStop(UART_HandleTypeDef *);
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *,uint16_t);
#endif
