#ifndef STM32TOOLS_RS485_CORE_H
#define STM32TOOLS_RS485_CORE_H
#include <stdint.h>
#include <cmsis_os2.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RS485_CORE_FRAME_MAX 256U
/* CMSIS-RTOS2 task-context frame cache. Zero-initialize before startup Init.
 * DMA/IDLE framing remains owned by UartReceive; Feed accepts those fragments.
 * The four-entry RX queue keeps the newest frames, as UartReceive does.
 * Lengths above FRAME_MAX are truncated. Timeouts are RTOS ticks.
 * One producer feeds frames, one transaction owner receives/flushes. */
typedef struct {
  osMutexId_t mutex;
  osMessageQueueId_t queue;
  uint8_t last_frame[RS485_CORE_FRAME_MAX];
  uint16_t last_length;
  uint32_t rx_count;
} Rs485Core;
typedef struct {
  void *ctx;
  void (*direction)(void *ctx, uint8_t transmit);
  uint8_t (*transmit)(void *ctx, const uint8_t *data, uint16_t length);
} Rs485Port;
uint8_t Rs485Core_Init(Rs485Core *core);
void Rs485Core_Feed(Rs485Core *core, const uint8_t *data, uint16_t length);
uint16_t Rs485Core_Receive(Rs485Core *core, uint8_t *out,
    uint16_t capacity, uint32_t timeout_ticks);
void Rs485Core_Flush(Rs485Core *core);
uint16_t Rs485Core_GetLastFrame(Rs485Core *core, uint8_t *out, uint16_t capacity);
uint32_t Rs485Core_GetRxCount(Rs485Core *core);
/* Synchronous transmit; always restores receive direction, including failure.
 * The caller owns bus serialization and enabled/safety checks. */
uint8_t Rs485Core_Send(const Rs485Port *port, const uint8_t *data, uint16_t length);
#ifdef __cplusplus
}
#endif
#endif
