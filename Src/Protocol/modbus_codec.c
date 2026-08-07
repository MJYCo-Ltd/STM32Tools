#include "Protocol/modbus_codec.h"

#include <string.h>

#include "Common.h"

size_t Modbus_AppendCrc(uint8_t *frame, size_t data_length,
                        size_t frame_capacity)
{
  const size_t frame_length = data_length + MODBUS_RTU_CRC_SIZE;

  if ((frame == NULL) || (data_length == 0U) ||
      (frame_length > frame_capacity) || (frame_length > UINT16_MAX)) {
    return 0U;
  }
  AddCRC16(frame, (uint16_t)frame_length, 1U);
  return frame_length;
}

size_t Modbus_BuildRequest(uint8_t address, const uint8_t *pdu,
                           size_t pdu_length, uint8_t *frame,
                           size_t frame_capacity)
{
  if ((pdu == NULL) || (pdu_length == 0U) || (frame == NULL) ||
      ((1U + pdu_length + MODBUS_RTU_CRC_SIZE) > frame_capacity)) {
    return 0U;
  }

  frame[0] = address;
  memcpy(frame + 1U, pdu, pdu_length);
  return Modbus_AppendCrc(frame, 1U + pdu_length, frame_capacity);
}

int Modbus_ValidateFrame(const uint8_t *frame, size_t frame_length)
{
  if ((frame == NULL) || (frame_length < 4U) || (frame_length > UINT16_MAX)) {
    return 0;
  }
  return JudgeCRC16(frame, (uint16_t)frame_length, 1U) != 0U;
}

int Modbus_ResponseMatches(const uint8_t *frame, size_t frame_length,
                           uint8_t address, uint8_t function)
{
  return Modbus_ValidateFrame(frame, frame_length) &&
         (frame[0] == address) && (frame[1] == function);
}
