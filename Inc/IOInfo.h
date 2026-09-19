#ifndef STM32TOOLS_IO_INFO_H
#define STM32TOOLS_IO_INFO_H
#include <stdint.h>
typedef struct {
    uint64_t unReciveCount;
    uint64_t unSendCount;
    uint64_t unDealCount;
} IOInfo;
#endif
