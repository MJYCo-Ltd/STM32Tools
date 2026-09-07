#ifndef STM32TOOLS_MODBUS_MASTER_H
#define STM32TOOLS_MODBUS_MASTER_H

#include <Protocol/ModbusRtu.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Callbacks run synchronously in the caller's task. The caller must serialize
 * complete transactions sharing a bus. Receive must not exceed capacity and
 * returns zero on timeout. All callbacks are required; ctx may be NULL. */
typedef struct {
  void *ctx;
  void (*flush)(void *ctx);
  uint8_t (*send)(void *ctx, const uint8_t *data, uint16_t length);
  uint16_t (*receive)(void *ctx, uint8_t *out, uint16_t capacity,
                      uint32_t timeout_ms);
} ModbusMasterTransport;

ModbusStatus ModbusMaster_Execute(const ModbusMasterTransport *transport,
                                  const ModbusRequest *request,
                                  ModbusResponse *response);

#ifdef __cplusplus
}
#endif
#endif /* STM32TOOLS_MODBUS_MASTER_H */
