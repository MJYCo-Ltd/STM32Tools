#ifndef STM32TOOLS_MODBUS_CODEC_H
#define STM32TOOLS_MODBUS_CODEC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MODBUS_RTU_CRC_SIZE 2U

size_t Modbus_AppendCrc(uint8_t *frame, size_t data_length,
                        size_t frame_capacity);
size_t Modbus_BuildRequest(uint8_t address, const uint8_t *pdu,
                           size_t pdu_length, uint8_t *frame,
                           size_t frame_capacity);
int Modbus_ValidateFrame(const uint8_t *frame, size_t frame_length);
int Modbus_ResponseMatches(const uint8_t *frame, size_t frame_length,
                           uint8_t address, uint8_t function);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_MODBUS_CODEC_H */
