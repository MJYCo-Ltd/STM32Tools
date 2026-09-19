#ifndef STM32TOOLS_IO_STATISTICS_H
#define STM32TOOLS_IO_STATISTICS_H
#include <stdint.h>
typedef struct {
  uint64_t unReciveCount; /* received bytes */
  uint64_t unSendCount;   /* transmitted bytes */
  uint64_t unDealCount;   /* delivered bytes */
} IOInfo;
#endif
