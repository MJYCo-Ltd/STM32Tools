#ifndef TEST_FREERTOS_H
#define TEST_FREERTOS_H
#include <stddef.h>
void *pvPortMalloc(size_t size);
void vPortFree(void *pointer);
size_t xPortGetFreeHeapSize(void);
#endif
