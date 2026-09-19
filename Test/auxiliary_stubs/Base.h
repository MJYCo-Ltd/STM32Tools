#ifndef AUX_TEST_BASE_H
#define AUX_TEST_BASE_H
#include <stdlib.h>
#define configTOTAL_HEAP_SIZE 65536U
#define YTY_MALLOC(n) malloc(n)
#define YTY_FREE(p) free(p)
#define YTY_DELAY_MS(n) ((void)(n))
size_t xPortGetFreeHeapSize(void);
#endif
