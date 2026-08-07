/**
 * Bind a W25Q_Device to StorageBackend (SPI NOR → storage stack).
 * Lock / unlock / voltage_ok are optional; pass NULL to leave unset.
 */
#ifndef STM32TOOLS_W25Q_STORAGE_H
#define STM32TOOLS_W25Q_STORAGE_H

#include "Flash/storage_backend.h"
#include "W25Q/w25q.h"

#ifdef __cplusplus
extern "C" {
#endif

void W25Q_InitStorageBackend(StorageBackend *backend, W25Q_Device *dev,
                             void (*lock)(void *ctx), void (*unlock)(void *ctx),
                             int (*voltage_ok)(void *ctx));

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_W25Q_STORAGE_H */
