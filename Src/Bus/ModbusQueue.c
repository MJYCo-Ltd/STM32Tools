#include <Bus/ModbusQueue.h>



#include "FreeRTOS.h"
#include "task.h"

#include <string.h>

#define MODBUS_BUS_REQUEST_DEPTH_PER_PRIORITY 4U
#define MODBUS_BUS_RESULT_DEPTH 8U


void ModbusQueue_GetCounters(ModbusQueue *bus, uint32_t *success_count, uint32_t *error_count)
{
  if (bus == NULL) return;
  taskENTER_CRITICAL();
  if (success_count != NULL) *success_count = bus->success_count;
  if (error_count != NULL) *error_count = bus->error_count;
  taskEXIT_CRITICAL();
}

uint8_t ModbusQueue_Init(ModbusQueue *bus, ModbusQueueExecuteFn execute, void *ctx)
{
  uint8_t i;
  if (bus == NULL || execute == NULL) return 0U;
  if (bus->ready) return 1U;
  bus->execute = execute;
  bus->ctx = ctx;

  for (i = 0U; i < 4U; ++i) {
    if (bus->requests[i] == NULL) {
      bus->requests[i] = osMessageQueueNew(
          MODBUS_BUS_REQUEST_DEPTH_PER_PRIORITY, sizeof(ModbusBusRequest),
          NULL);
    }
  }
  if (bus->results == NULL) {
    bus->results = osMessageQueueNew(MODBUS_BUS_RESULT_DEPTH,
                                       sizeof(ModbusBusResult), NULL);
  }
  if (bus->results == NULL) {
    return 0U;
  }
  for (i = 0U; i < 4U; ++i) {
    if (bus->requests[i] == NULL) {
      return 0U;
    }
  }
  bus->ready = 1U;
  return 1U;
}

ModbusStatus ModbusQueue_Submit(ModbusQueue *bus, const ModbusBusRequest *request,
                              uint8_t priority)
{
  if ((bus == NULL) || (request == NULL) || (request->retries == UINT8_MAX) ||
      (priority > 3U)) {
    return MODBUS_ERR_ARGUMENT;
  }
  if (!bus->ready || bus->requests[priority] == NULL) return MODBUS_ERR_BUSY;
  return (osMessageQueuePut(bus->requests[(uint8_t)priority], request, 0U,
                            0U) == osOK)
             ? MODBUS_OK
             : MODBUS_ERR_BUSY;
}

uint8_t ModbusQueue_GetResult(ModbusQueue *bus, ModbusBusResult *result, uint32_t timeout_ms)
{
  if ((bus == NULL) || (result == NULL) || (bus->results == NULL)) {
    return 0U;
  }
  return (osMessageQueueGet(bus->results, result, NULL, timeout_ms) == osOK)
             ? 1U
             : 0U;
}

uint8_t ModbusQueue_ProcessOne(ModbusQueue *bus, uint32_t wait_ms)
{
  ModbusBusRequest request;
  ModbusBusResult result;
  uint8_t attempt;
  int8_t priority;

  if (bus == NULL || bus->results == NULL || bus->execute == NULL || !bus->ready) return 0U;
  if (bus->have_pending) {
    if (osMessageQueuePut(bus->results, &bus->pending, 0U, 0U) != osOK) return 0U;
    bus->have_pending = 0U;
  }
  if (osMessageQueueGetSpace(bus->results) == 0U) return 0U;
  for (priority = (int8_t)3U;
       priority >= (int8_t)0U; --priority) {
    if (osMessageQueueGet(bus->requests[(uint8_t)priority], &request, NULL,
                          0U) == osOK) {
      break;
    }
  }
  if (priority < (int8_t)0U) {
    if (wait_ms != 0U) {
      osDelay(wait_ms);
    }
    return 0U;
  }
  memset(&result, 0, sizeof(result));
  result.request_id = request.request_id;
  for (attempt = 0U; attempt <= request.retries; ++attempt) {
    result.attempts = (uint8_t)(attempt + 1U);
    if ((result.response.status = bus->execute(bus->ctx, &request.request, &result.response)) == MODBUS_OK) {
      break;
    }
    if ((result.response.status != MODBUS_ERR_TIMEOUT) &&
        (result.response.status != MODBUS_ERR_CRC) &&
        (result.response.status != MODBUS_ERR_TRANSPORT)) {
      break;
    }
  }
  taskENTER_CRITICAL();
  if (result.response.status == MODBUS_OK) ++bus->success_count;
  else ++bus->error_count;
  taskEXIT_CRITICAL();
  if (osMessageQueuePut(bus->results, &result, 0U, 0U) != osOK) {
    bus->pending = result;
    bus->have_pending = 1U;
  }
  return 1U;
}
