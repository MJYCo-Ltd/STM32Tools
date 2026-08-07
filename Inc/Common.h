#ifndef STM32TOOLS_COMMON_H
#define STM32TOOLS_COMMON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t CalCRC16(const uint8_t *buffer, uint16_t length);
void AddCRC16(uint8_t *buffer, uint16_t length, uint8_t little_endian);
uint8_t JudgeCRC16(const uint8_t *buffer, uint16_t length,
                   uint8_t little_endian);

/** IEEE 802.3 CRC-32 (poly 0xEDB88320, init/xor 0xFFFFFFFF). */
uint32_t CalCRC32(const void *data, size_t length);
uint32_t CalCRC32Update(uint32_t crc, const void *data, size_t length);

uint16_t ReadBE16(const uint8_t *data);
uint32_t ReadBE32(const uint8_t *data);
uint16_t ReadLE16(const uint8_t *data);
void WriteBE16(uint8_t *data, uint16_t value);
void WriteBE32(uint8_t *data, uint32_t value);
void WriteLE16(uint8_t *data, uint16_t value);

/** Return an aligned pseudo-random value in [start, end). */
uint32_t Rand_range(uint32_t start, uint32_t end, uint32_t align);

static inline void Swap(uint16_t *a, uint16_t *b)
{
  uint16_t temporary = *a;
  *a = *b;
  *b = temporary;
}

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_COMMON_H */
