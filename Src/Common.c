#include "Common.h"

#include <stdlib.h>

uint16_t CalCRC16(const uint8_t *buffer, uint16_t length)
{
  uint16_t crc = 0xFFFFU;
  uint16_t index;
  uint8_t bit;

  if (buffer == NULL) {
    return 0U;
  }
  for (index = 0U; index < length; ++index) {
    crc ^= buffer[index];
    for (bit = 0U; bit < 8U; ++bit) {
      crc = ((crc & 1U) != 0U) ? (uint16_t)((crc >> 1) ^ 0xA001U)
                               : (uint16_t)(crc >> 1);
    }
  }
  return crc;
}

void AddCRC16(uint8_t *buffer, uint16_t length, uint8_t little_endian)
{
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
                   uint8_t little_endian)
{
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

uint32_t CalCRC32Update(uint32_t crc, const void *data, size_t length)
{
  const uint8_t *bytes = (const uint8_t *)data;
  size_t i;
  uint8_t bit;

  if ((data == NULL) && (length != 0U)) {
    return crc;
  }
  for (i = 0U; i < length; ++i) {
    crc ^= bytes[i];
    for (bit = 0U; bit < 8U; ++bit) {
      if ((crc & 1U) != 0U) {
        crc = (crc >> 1U) ^ 0xEDB88320UL;
      } else {
        crc >>= 1U;
      }
    }
  }
  return crc;
}

uint32_t CalCRC32(const void *data, size_t length)
{
  return CalCRC32Update(0xFFFFFFFFUL, data, length) ^ 0xFFFFFFFFUL;
}

uint16_t ReadBE16(const uint8_t *data)
{
  return ((uint16_t)data[0] << 8) | data[1];
}

uint32_t ReadBE32(const uint8_t *data)
{
  return ((uint32_t)ReadBE16(data) << 16) | ReadBE16(data + 2U);
}

uint16_t ReadLE16(const uint8_t *data)
{
  return ((uint16_t)data[1] << 8) | data[0];
}

void WriteBE16(uint8_t *data, uint16_t value)
{
  data[0] = (uint8_t)(value >> 8);
  data[1] = (uint8_t)value;
}

void WriteBE32(uint8_t *data, uint32_t value)
{
  WriteBE16(data, (uint16_t)(value >> 16));
  WriteBE16(data + 2U, (uint16_t)value);
}

void WriteLE16(uint8_t *data, uint16_t value)
{
  data[0] = (uint8_t)value;
  data[1] = (uint8_t)(value >> 8);
}

uint32_t Rand_range(uint32_t start, uint32_t end, uint32_t align)
{
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
