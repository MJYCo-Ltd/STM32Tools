#include <Bus/Rs485Core.h>
#include <string.h>
typedef struct {
  uint16_t length;
  uint8_t data[RS485_CORE_FRAME_MAX];
} Rs485RxFrame;
uint8_t Rs485Core_Init(Rs485Core *core)
{
  if (core == NULL) return 0U;
  if (core->mutex == NULL) core->mutex = osMutexNew(NULL);
  if (core->queue == NULL) core->queue = osMessageQueueNew(4U, sizeof(Rs485RxFrame), NULL);
  return (core->mutex != NULL && core->queue != NULL) ? 1U : 0U;
}
uint8_t Rs485Core_Send(const Rs485Port *port, const uint8_t *data, uint16_t length)
{
  uint8_t status;
  if (port == NULL || port->direction == NULL || port->transmit == NULL ||
      data == NULL || length == 0U) return 0U;
  port->direction(port->ctx, 1U);
  status = port->transmit(port->ctx, data, length);
  port->direction(port->ctx, 0U);
  return status;
}
void Rs485Core_Feed(Rs485Core *core, const uint8_t *data, uint16_t length)
{
  uint16_t copy_length;
  Rs485RxFrame frame;

  if ((core == NULL) || (data == NULL) || (length == 0U) ||
      (core->mutex == NULL)) {
    return;
  }

  copy_length = length;
  if (copy_length > RS485_CORE_FRAME_MAX) {
    copy_length = RS485_CORE_FRAME_MAX;
  }
  if (osMutexAcquire(core->mutex, osWaitForever) == osOK) {
    memcpy(core->last_frame, data, copy_length);
    core->last_length = copy_length;
    ++core->rx_count;
    (void)osMutexRelease(core->mutex);
  }
  if (core->queue != NULL) {
    frame.length = copy_length;
    memcpy(frame.data, data, copy_length);
    if (osMessageQueuePut(core->queue, &frame, 0U, 0U) != osOK) {
      Rs485RxFrame discarded;
      (void)osMessageQueueGet(core->queue, &discarded, NULL, 0U);
      (void)osMessageQueuePut(core->queue, &frame, 0U, 0U);
    }
  }
}

uint16_t Rs485Core_Receive(Rs485Core *core, uint8_t *out, uint16_t max_length,
                           uint32_t timeout_ms)
{
  Rs485RxFrame frame;
  uint16_t copy_length;

  if ((core == NULL) || (out == NULL) || (max_length == 0U) || (core->queue == NULL)) {
    return 0U;
  }
  if (osMessageQueueGet(core->queue, &frame, NULL, timeout_ms) != osOK) {
    return 0U;
  }
  copy_length = frame.length;
  if (copy_length > max_length) {
    copy_length = max_length;
  }
  memcpy(out, frame.data, copy_length);
  return copy_length;
}

void Rs485Core_Flush(Rs485Core *core)
{
  Rs485RxFrame frame;

  if (core == NULL || core->queue == NULL) {
    return;
  }
  while (osMessageQueueGet(core->queue, &frame, NULL, 0U) == osOK) {
  }
}

uint16_t Rs485Core_GetLastFrame(Rs485Core *core, uint8_t *out, uint16_t max_length)
{
  uint16_t copy_length;

  if ((core == NULL) || (out == NULL) || (max_length == 0U) || (core->mutex == NULL)) {
    return 0U;
  }
  if (osMutexAcquire(core->mutex, osWaitForever) != osOK) {
    return 0U;
  }
  if (core->last_length == 0U) {
    (void)osMutexRelease(core->mutex);
    return 0U;
  }
  copy_length = core->last_length;
  if (copy_length > max_length) {
    copy_length = max_length;
  }
  memcpy(out, core->last_frame, copy_length);
  (void)osMutexRelease(core->mutex);
  return copy_length;
}

uint32_t Rs485Core_GetRxCount(Rs485Core *core)
{
  uint32_t count;

  if ((core == NULL) || (core->mutex == NULL) ||
      (osMutexAcquire(core->mutex, osWaitForever) != osOK)) {
    return 0U;
  }
  count = core->rx_count;
  (void)osMutexRelease(core->mutex);
  return count;
}
