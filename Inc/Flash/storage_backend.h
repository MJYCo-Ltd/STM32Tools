#ifndef STM32TOOLS_STORAGE_BACKEND_H
#define STM32TOOLS_STORAGE_BACKEND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  STORAGE_OK = 0,
  STORAGE_ERR_PARAM,
  STORAGE_ERR_RANGE,
  STORAGE_ERR_IO,
  STORAGE_ERR_TIMEOUT,
  STORAGE_ERR_BUSY,
  STORAGE_ERR_CRC,
  STORAGE_ERR_STATE,
  STORAGE_ERR_NO_SPACE,
  STORAGE_ERR_READONLY,
  STORAGE_ERR_VOLTAGE,
  STORAGE_ERR_NOT_FOUND,
  STORAGE_ERR_INJECT
} Storage_Status;

typedef struct StorageBackend {
  void *ctx;
  Storage_Status (*read)(void *ctx, uint32_t address, void *buffer,
                         uint32_t length);
  Storage_Status (*write)(void *ctx, uint32_t address, const void *buffer,
                          uint32_t length);
  Storage_Status (*erase_sector)(void *ctx, uint32_t address);
  Storage_Status (*erase_block64)(void *ctx, uint32_t address);
  void (*lock)(void *ctx);
  void (*unlock)(void *ctx);
  /** Return 1 if erase/write is allowed, 0 to deny. NULL means always OK. */
  int (*voltage_ok)(void *ctx);
} StorageBackend;

void StorageBackend_Lock(const StorageBackend *backend);
void StorageBackend_Unlock(const StorageBackend *backend);
Storage_Status StorageBackend_CheckVoltage(const StorageBackend *backend);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_STORAGE_BACKEND_H */
