#include "Common.h"

#include <stdlib.h>

void ReverseByteOrder(const uint8_t *input, uint8_t *output, uint8_t length) {
  uint8_t i;
  if ((input == NULL) || (output == NULL) || (length == 0U)) {
    return;
  }
  for (i = 0U; i < length; ++i) {
    output[i] = input[length - 1U - i];
  }
}

uint16_t CalCRC16(const uint8_t *buffer, uint16_t length) {
  uint16_t crc = 0xFFFFU;
  uint16_t i;
  uint8_t bit;

  if (buffer == NULL) {
    return 0U;
  }
  for (i = 0U; i < length; ++i) {
    crc ^= buffer[i];
    for (bit = 0U; bit < 8U; ++bit) {
      crc = ((crc & 1U) != 0U) ? (uint16_t)((crc >> 1) ^ 0xA001U)
                               : (uint16_t)(crc >> 1);
    }
  }
  return crc;
}

void AddCRC16(uint8_t *buffer, uint16_t length, uint8_t little_endian) {
  uint16_t crc;
  if ((buffer == NULL) || (length < 2U)) {
    return;
  }
  crc = CalCRC16(buffer, length - 2U);
  if (little_endian != 0U) {
    WriteLE16(buffer + length - 2U, crc);
  } else {
    WriteBE16(buffer + length - 2U, crc);
  }
}

uint8_t JudgeCRC16(const uint8_t *buffer, uint16_t length,
                   uint8_t little_endian) {
  uint16_t expected;
  uint16_t actual;
  if ((buffer == NULL) || (length < 2U) || (little_endian > 1U)) {
    return 0U;
  }
  expected = CalCRC16(buffer, length - 2U);
  actual = (little_endian != 0U) ? ReadLE16(buffer + length - 2U)
                                 : ReadBE16(buffer + length - 2U);
  return (expected == actual) ? 1U : 0U;
}

uint16_t ReadBE16(const uint8_t *data) {
  return ((uint16_t)data[0] << 8) | (uint16_t)data[1];
}

uint32_t ReadBE32(const uint8_t *data) {
  return ((uint32_t)ReadBE16(data) << 16) | ReadBE16(data + 2);
}

uint64_t ReadBE64(const uint8_t *data) {
  return ((uint64_t)ReadBE32(data) << 32) | ReadBE32(data + 4);
}

uint16_t ReadLE16(const uint8_t *data) {
  return ((uint16_t)data[1] << 8) | (uint16_t)data[0];
}

uint32_t ReadLE32(const uint8_t *data) {
  return ((uint32_t)ReadLE16(data + 2) << 16) | ReadLE16(data);
}

uint64_t ReadLE64(const uint8_t *data) {
  return ((uint64_t)ReadLE32(data + 4) << 32) | ReadLE32(data);
}

void WriteBE16(uint8_t *data, uint16_t value) {
  data[0] = (uint8_t)(value >> 8);
  data[1] = (uint8_t)value;
}

void WriteBE32(uint8_t *data, uint32_t value) {
  WriteBE16(data, (uint16_t)(value >> 16));
  WriteBE16(data + 2, (uint16_t)value);
}

void WriteBE64(uint8_t *data, uint64_t value) {
  WriteBE32(data, (uint32_t)(value >> 32));
  WriteBE32(data + 4, (uint32_t)value);
}

void WriteLE16(uint8_t *data, uint16_t value) {
  data[0] = (uint8_t)value;
  data[1] = (uint8_t)(value >> 8);
}

void WriteLE32(uint8_t *data, uint32_t value) {
  WriteLE16(data, (uint16_t)value);
  WriteLE16(data + 2, (uint16_t)(value >> 16));
}

void WriteLE64(uint8_t *data, uint64_t value) {
  WriteLE32(data, (uint32_t)value);
  WriteLE32(data + 4, (uint32_t)(value >> 32));
}

void ConvertBigEndian2Double(const uint8_t *data, DOUBLE_DATA *out) {
  out->u64Data = ReadBE64(data);
}
void ConvertBigEndian2Word(const uint8_t *data, WORD_DATA *out) {
  out->u32Data = ReadBE32(data);
}
void ConvertBigEndian2HalfWord(const uint8_t *data, HALF_WORD_DATA *out) {
  out->u16Data = ReadBE16(data);
}
void ConvertDouble2BigEndian(const DOUBLE_DATA *value, uint8_t *out) {
  WriteBE64(out, value->u64Data);
}
void ConvertWord2BigEndian(const WORD_DATA *value, uint8_t *out) {
  WriteBE32(out, value->u32Data);
}
void ConvertHalfWord2BigEndian(const HALF_WORD_DATA *value, uint8_t *out) {
  WriteBE16(out, value->u16Data);
}
void ConvertLittleEndian2Double(const uint8_t *data, DOUBLE_DATA *out) {
  out->u64Data = ReadLE64(data);
}
void ConvertLittleEndian2Word(const uint8_t *data, WORD_DATA *out) {
  out->u32Data = ReadLE32(data);
}
void ConvertLittleEndian2HalfWord(const uint8_t *data, HALF_WORD_DATA *out) {
  out->u16Data = ReadLE16(data);
}
void ConvertDouble2LittleEndian(const DOUBLE_DATA *value, uint8_t *out) {
  WriteLE64(out, value->u64Data);
}
void ConvertWord2LitteleEndian(const WORD_DATA *value, uint8_t *out) {
  WriteLE32(out, value->u32Data);
}
void ConvertHalfWord2LitteleEndian(const HALF_WORD_DATA *value, uint8_t *out) {
  WriteLE16(out, value->u16Data);
}

uint32_t Rand_range(uint32_t start, uint32_t end, uint32_t align) {
  uint32_t first;
  uint32_t last;
  uint32_t count;

  if ((align == 0U) || ((align & (align - 1U)) != 0U) || (start >= end)) {
    return start;
  }
  first = (start + align - 1U) & ~(align - 1U);
  last = (end - 1U) & ~(align - 1U);
  if (first > last) {
    return first;
  }
  count = ((last - first) / align) + 1U;
  return first + ((uint32_t)rand() % count) * align;
}
