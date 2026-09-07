#include <Common.h>
#include <Protocol/ModbusRtu.h>

#include <string.h>

#include <Protocol/modbus_codec.h>



static void AppendCrc(uint8_t *frame, uint16_t payload_length)
{
  (void)Modbus_AppendCrc(frame, payload_length,
                         (size_t)payload_length + MODBUS_RTU_CRC_SIZE);
}

ModbusStatus ModbusRtu_BuildRequest(const ModbusRequest *request, uint8_t *out,
                                    uint16_t capacity,
                                    uint16_t *length_out)
{
  uint16_t length = 8U;
  uint16_t i;

  if ((request == NULL) || (out == NULL) || (length_out == NULL) ||
      (request->address == 0U) || (request->address > 247U)) {
    return MODBUS_ERR_ARGUMENT;
  }
  if ((request->function == MODBUS_FC_READ_COILS) ||
      (request->function == MODBUS_FC_READ_HOLDING_REGISTERS) ||
      (request->function == MODBUS_FC_READ_INPUT_REGISTERS)) {
    if ((request->quantity == 0U) ||
        (request->quantity > MODBUS_RTU_MAX_REGISTERS)) {
      return MODBUS_ERR_ARGUMENT;
    }
  } else if ((request->function == MODBUS_FC_WRITE_SINGLE_COIL) ||
             (request->function == MODBUS_FC_WRITE_SINGLE_REGISTER)) {
    if ((request->function == MODBUS_FC_WRITE_SINGLE_COIL) &&
        (request->value != 0x0000U) && (request->value != 0xFF00U)) {
      return MODBUS_ERR_ARGUMENT;
    }
  } else if (request->function == MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
    if ((request->quantity == 0U) ||
        (request->quantity > MODBUS_RTU_MAX_WRITE_REGISTERS)) {
      return MODBUS_ERR_ARGUMENT;
    }
    length = (uint16_t)(9U + request->quantity * 2U);
  } else {
    return MODBUS_ERR_FUNCTION;
  }
  if (capacity < length) {
    return MODBUS_ERR_ARGUMENT;
  }

  out[0] = request->address;
  out[1] = (uint8_t)request->function;
  WriteBE16(&out[2], request->start);
  if (request->function == MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
    WriteBE16(&out[4], request->quantity);
    out[6] = (uint8_t)(request->quantity * 2U);
    for (i = 0U; i < request->quantity; ++i) {
      WriteBE16(&out[7U + i * 2U], request->values[i]);
    }
  } else if ((request->function == MODBUS_FC_WRITE_SINGLE_COIL) ||
             (request->function == MODBUS_FC_WRITE_SINGLE_REGISTER)) {
    WriteBE16(&out[4], request->value);
  } else {
    WriteBE16(&out[4], request->quantity);
  }
  AppendCrc(out, (uint16_t)(length - 2U));
  *length_out = length;
  return MODBUS_OK;
}

ModbusStatus ModbusRtu_ParseResponse(const ModbusRequest *request,
                                     const uint8_t *frame, uint16_t length,
                                     ModbusResponse *response)
{
  uint16_t i;
  uint8_t byte_count;

  if ((request == NULL) || (frame == NULL) || (response == NULL)) {
    return MODBUS_ERR_ARGUMENT;
  }
  memset(response, 0, sizeof(*response));
  if (length < 5U) {
    return MODBUS_ERR_FRAME;
  }
  if (Modbus_ValidateFrame(frame, length) == 0) {
    return MODBUS_ERR_CRC;
  }
  if (frame[0] != request->address) {
    return MODBUS_ERR_ADDRESS;
  }
  response->address = frame[0];
  if (frame[1] == ((uint8_t)request->function | 0x80U)) {
    if (length != 5U) {
      return MODBUS_ERR_FRAME;
    }
    response->function = request->function;
    response->exception_code = frame[2];
    response->status = MODBUS_ERR_EXCEPTION;
    return MODBUS_ERR_EXCEPTION;
  }
  if (frame[1] != (uint8_t)request->function) {
    return MODBUS_ERR_FUNCTION;
  }
  response->function = request->function;

  if ((request->function == MODBUS_FC_READ_HOLDING_REGISTERS) ||
      (request->function == MODBUS_FC_READ_INPUT_REGISTERS)) {
    byte_count = frame[2];
    if ((byte_count != request->quantity * 2U) ||
        (length != (uint16_t)(byte_count + 5U))) {
      return MODBUS_ERR_FRAME;
    }
    response->quantity = request->quantity;
    for (i = 0U; i < request->quantity; ++i) {
      response->registers[i] = ReadBE16(&frame[3U + i * 2U]);
    }
  } else if (request->function == MODBUS_FC_READ_COILS) {
    byte_count = frame[2];
    if ((byte_count != (uint8_t)((request->quantity + 7U) / 8U)) ||
        (length != (uint16_t)(byte_count + 5U))) {
      return MODBUS_ERR_FRAME;
    }
    response->quantity = request->quantity;
    memcpy(response->bits, &frame[3], byte_count);
  } else {
    if ((length != 8U) || (ReadBE16(&frame[2]) != request->start)) {
      return MODBUS_ERR_FRAME;
    }
    if (request->function == MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
      if (ReadBE16(&frame[4]) != request->quantity) {
        return MODBUS_ERR_FRAME;
      }
    } else if (ReadBE16(&frame[4]) != request->value) {
      return MODBUS_ERR_FRAME;
    }
  }
  response->status = MODBUS_OK;
  return MODBUS_OK;
}
