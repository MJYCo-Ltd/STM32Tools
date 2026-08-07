#include "W25Q/w25q_storage.h"

#include <string.h>

static Storage_Status MapW25q(W25Q_Status st)
{
  switch (st) {
  case W25Q_OK:
    return STORAGE_OK;
  case W25Q_ERR_PARAM:
    return STORAGE_ERR_PARAM;
  case W25Q_ERR_TIMEOUT:
    return STORAGE_ERR_TIMEOUT;
  case W25Q_ERR_RANGE:
    return STORAGE_ERR_RANGE;
  default:
    return STORAGE_ERR_IO;
  }
}

static Storage_Status BackendRead(void *ctx, uint32_t address, void *buffer,
                                  uint32_t length)
{
  return MapW25q(W25Q_Read((W25Q_Device *)ctx, address, (uint8_t *)buffer,
                           length));
}

static Storage_Status BackendWrite(void *ctx, uint32_t address,
                                   const void *buffer, uint32_t length)
{
  return MapW25q(W25Q_Write((W25Q_Device *)ctx, address,
                            (const uint8_t *)buffer, length));
}

static Storage_Status BackendEraseSector(void *ctx, uint32_t address)
{
  return MapW25q(W25Q_EraseSector((W25Q_Device *)ctx, address));
}

static Storage_Status BackendEraseBlock64(void *ctx, uint32_t address)
{
  return MapW25q(W25Q_EraseBlock64((W25Q_Device *)ctx, address));
}

void W25Q_InitStorageBackend(StorageBackend *backend, W25Q_Device *dev,
                             void (*lock)(void *ctx), void (*unlock)(void *ctx),
                             int (*voltage_ok)(void *ctx))
{
  if (backend == NULL) {
    return;
  }
  memset(backend, 0, sizeof(*backend));
  if (dev == NULL) {
    return;
  }
  backend->ctx = dev;
  backend->read = BackendRead;
  backend->write = BackendWrite;
  backend->erase_sector = BackendEraseSector;
  backend->erase_block64 = BackendEraseBlock64;
  backend->lock = lock;
  backend->unlock = unlock;
  backend->voltage_ok = voltage_ok;
}
