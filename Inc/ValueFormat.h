#ifndef STM32TOOLS_VALUE_FORMAT_H
#define STM32TOOLS_VALUE_FORMAT_H

#include <stddef.h>
#include <stdint.h>

/** Format a signed 64-bit integer without relying on embedded printf long-long support. */
uint8_t ValueFormat_Int64(char *buffer, size_t capacity, int64_t value);

#endif /* STM32TOOLS_VALUE_FORMAT_H */
