#ifndef UART_TEST_CMSIS_H
#define UART_TEST_CMSIS_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
typedef void *osMessageQueueId_t;
typedef int osStatus_t;
#define osOK 0
#define osErrorResource -1
#define pvPortMalloc malloc
#define vPortFree free
osMessageQueueId_t osMessageQueueNew(uint32_t, uint32_t, const void *);
osStatus_t osMessageQueuePut(osMessageQueueId_t, const void *, uint8_t, uint32_t);
osStatus_t osMessageQueueGet(osMessageQueueId_t, void *, uint8_t *, uint32_t);
osStatus_t osMessageQueueDelete(osMessageQueueId_t);
uint32_t osMessageQueueGetCount(osMessageQueueId_t);
#endif
