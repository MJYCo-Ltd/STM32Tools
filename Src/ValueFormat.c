#include <ValueFormat.h>

uint8_t ValueFormat_Int64(char *buffer, size_t capacity, int64_t value)
{
  char reversed[20];
  uint64_t magnitude;
  size_t digits = 0U;
  size_t output = 0U;

  if ((buffer == NULL) || (capacity == 0U)) return 0U;
  magnitude = (value < 0) ? (uint64_t)(-(value + 1)) + 1U : (uint64_t)value;
  do {
    reversed[digits++] = (char)('0' + (magnitude % 10U));
    magnitude /= 10U;
  } while (magnitude != 0U);
  if (digits + ((value < 0) ? 1U : 0U) + 1U > capacity) {
    buffer[0] = '\0';
    return 0U;
  }
  if (value < 0) buffer[output++] = '-';
  while (digits != 0U) buffer[output++] = reversed[--digits];
  buffer[output] = '\0';
  return 1U;
}
