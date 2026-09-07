#ifndef STM32TOOLS_MODBUS_RTU_H
#define STM32TOOLS_MODBUS_RTU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MODBUS_RTU_ADU_MAX 256U
#define MODBUS_RTU_MAX_REGISTERS 64U
#define MODBUS_RTU_MAX_WRITE_REGISTERS 32U

typedef enum {
  MODBUS_OK = 0,
  MODBUS_ERR_ARGUMENT,
  MODBUS_ERR_BUSY,
  MODBUS_ERR_TRANSPORT,
  MODBUS_ERR_TIMEOUT,
  MODBUS_ERR_FRAME,
  MODBUS_ERR_CRC,
  MODBUS_ERR_ADDRESS,
  MODBUS_ERR_FUNCTION,
  MODBUS_ERR_EXCEPTION
} ModbusStatus;

typedef enum {
  MODBUS_FC_READ_COILS = 0x01,
  MODBUS_FC_READ_HOLDING_REGISTERS = 0x03,
  MODBUS_FC_READ_INPUT_REGISTERS = 0x04,
  MODBUS_FC_WRITE_SINGLE_COIL = 0x05,
  MODBUS_FC_WRITE_SINGLE_REGISTER = 0x06,
  MODBUS_FC_WRITE_MULTIPLE_REGISTERS = 0x10
} ModbusFunction;

typedef struct {
  uint8_t address;
  ModbusFunction function;
  uint16_t start;
  uint16_t quantity;
  uint16_t value;
  uint16_t values[MODBUS_RTU_MAX_WRITE_REGISTERS];
  uint32_t timeout_ms;
} ModbusRequest;

typedef struct {
  ModbusStatus status;
  uint8_t address;
  ModbusFunction function;
  uint8_t exception_code;
  uint16_t quantity;
  uint16_t registers[MODBUS_RTU_MAX_REGISTERS];
  uint8_t bits[(MODBUS_RTU_MAX_REGISTERS + 7U) / 8U];
} ModbusResponse;

ModbusStatus ModbusRtu_BuildRequest(const ModbusRequest *request, uint8_t *out,
                                    uint16_t capacity,
                                    uint16_t *length_out);
ModbusStatus ModbusRtu_ParseResponse(const ModbusRequest *request,
                                     const uint8_t *frame, uint16_t length,
                                     ModbusResponse *response);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_MODBUS_RTU_H */
