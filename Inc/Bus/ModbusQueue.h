#ifndef STM32TOOLS_MODBUS_QUEUE_H
#define STM32TOOLS_MODBUS_QUEUE_H
#include <Protocol/ModbusRtu.h>
#include <cmsis_os2.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
  uint32_t request_id;
  uint8_t retries;
  uint8_t reserved[3];
  ModbusRequest request;
} ModbusBusRequest;

typedef struct {
  uint32_t request_id;
  uint8_t attempts;
  uint8_t reserved[3];
  ModbusResponse response;
} ModbusBusResult;

/* CMSIS-RTOS2 / FreeRTOS. Zero-initialize before startup-only Init.
 * One worker calls ProcessOne; priority 3 is highest. Multiple submitters and
 * result consumers are supported. Execute must serialize with other bus users.
 * Timeouts are RTOS ticks. Completed results are never overwritten.
 * retries=255 is rejected because attempts is an 8-bit count. */
typedef ModbusStatus (*ModbusQueueExecuteFn)(void *ctx,
    const ModbusRequest *request, ModbusResponse *response);
typedef struct {
  osMessageQueueId_t requests[4];
  osMessageQueueId_t results;
  ModbusQueueExecuteFn execute;
  void *ctx;
  uint32_t success_count, error_count;
  ModbusBusResult pending;
  uint8_t have_pending, ready;
} ModbusQueue;
uint8_t ModbusQueue_Init(ModbusQueue *bus, ModbusQueueExecuteFn execute, void *ctx);
ModbusStatus ModbusQueue_Submit(ModbusQueue *bus,
    const ModbusBusRequest *request, uint8_t priority);
uint8_t ModbusQueue_GetResult(ModbusQueue *bus, ModbusBusResult *result,
    uint32_t timeout_ticks);
uint8_t ModbusQueue_ProcessOne(ModbusQueue *bus, uint32_t wait_ticks);
void ModbusQueue_GetCounters(ModbusQueue *bus, uint32_t *success, uint32_t *error);
#ifdef __cplusplus
}
#endif
#endif
