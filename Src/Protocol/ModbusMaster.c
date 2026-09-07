#include <Protocol/ModbusMaster.h>

#include <string.h>

#define MODBUS_DEFAULT_TIMEOUT_MS 500U
#define MODBUS_FRAGMENT_TIMEOUT_MS 10U

static uint16_t ExpectedLength(const ModbusRequest *request,
                               const uint8_t *frame, uint16_t length)
{
  if (length < 2U) {
    return 5U;
  }
  if (frame[1] == ((uint8_t)request->function | 0x80U)) {
    return 5U;
  }
  if ((request->function == MODBUS_FC_READ_COILS) ||
      (request->function == MODBUS_FC_READ_HOLDING_REGISTERS) ||
      (request->function == MODBUS_FC_READ_INPUT_REGISTERS)) {
    return (length >= 3U) ? (uint16_t)(frame[2] + 5U) : 5U;
  }
  return 8U;
}

ModbusStatus ModbusMaster_Execute(const ModbusMasterTransport *transport,
                                  const ModbusRequest *request,
                                  ModbusResponse *response)
{
  uint8_t tx[MODBUS_RTU_ADU_MAX];
  uint8_t rx[MODBUS_RTU_ADU_MAX];
  uint16_t tx_length;
  uint16_t rx_length;
  uint16_t expected_length;
  uint16_t fragment_length;
  uint32_t timeout;
  ModbusStatus status;

  if ((request == NULL) || (response == NULL)) {
    return MODBUS_ERR_ARGUMENT;
  }
  memset(response, 0, sizeof(*response));
  if ((transport == NULL) || (transport->flush == NULL) ||
      (transport->send == NULL) || (transport->receive == NULL)) {
    response->status = MODBUS_ERR_ARGUMENT;
    return MODBUS_ERR_ARGUMENT;
  }
  status = ModbusRtu_BuildRequest(request, tx, sizeof(tx), &tx_length);
  if (status != MODBUS_OK) {
    response->status = status;
    return status;
  }
  transport->flush(transport->ctx);
  if (transport->send(transport->ctx, tx, tx_length) == 0U) {
    response->status = MODBUS_ERR_TRANSPORT;
    return MODBUS_ERR_TRANSPORT;
  }
  timeout = (request->timeout_ms != 0U) ? request->timeout_ms
                                        : MODBUS_DEFAULT_TIMEOUT_MS;
  rx_length = transport->receive(transport->ctx, rx, sizeof(rx), timeout);
  if (rx_length == 0U) {
    response->status = MODBUS_ERR_TIMEOUT;
    return MODBUS_ERR_TIMEOUT;
  }
  expected_length = ExpectedLength(request, rx, rx_length);
  while ((rx_length < expected_length) && (rx_length < sizeof(rx))) {
    fragment_length = transport->receive(transport->ctx,
        &rx[rx_length], (uint16_t)(sizeof(rx) - rx_length),
        MODBUS_FRAGMENT_TIMEOUT_MS);
    if (fragment_length == 0U) {
      break;
    }
    rx_length = (uint16_t)(rx_length + fragment_length);
    expected_length = ExpectedLength(request, rx, rx_length);
  }
  status = ModbusRtu_ParseResponse(request, rx, rx_length, response);
  response->status = status;
  return status;
}
