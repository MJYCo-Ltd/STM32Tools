#ifndef UART_TEST_MAIN_H
#define UART_TEST_MAIN_H
#include <stdint.h>
typedef enum { HAL_OK = 0, HAL_ERROR, HAL_BUSY } HAL_StatusTypeDef;
typedef struct {
  int dummy;
} DMA_HandleTypeDef;
typedef struct {
  uint32_t RxState, ErrorCode;
  DMA_HandleTypeDef *hdmarx;
  uint8_t *buffer;
  unsigned id;
} UART_HandleTypeDef;
#define HAL_UART_STATE_READY 0U
#define HAL_UART_STATE_BUSY_RX 1U
#define HAL_UART_ERROR_NONE 0U
#define DMA_IT_HT 1U
#define __HAL_UART_CLEAR_OREFLAG(u) ((void)(u))
#define __HAL_DMA_DISABLE_IT(d, m) ((void)(d), (void)(m))
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t);
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *, uint8_t *, uint16_t);
HAL_StatusTypeDef HAL_UART_DMAStop(UART_HandleTypeDef *);
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *, uint16_t);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *);
#endif
