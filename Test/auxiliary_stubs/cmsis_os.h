#ifndef AUXILIARY_TEST_RTOS_H
#define AUXILIARY_TEST_RTOS_H
#include <stddef.h>
#include <stdlib.h>
#define configTOTAL_HEAP_SIZE 65536U
#define pvPortMalloc malloc
#define vPortFree free
size_t xPortGetFreeHeapSize(void);
#endif
