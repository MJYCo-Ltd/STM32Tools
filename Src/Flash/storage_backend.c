#include "Flash/storage_backend.h"

#include <stddef.h>

void StorageBackend_Poll(const StorageBackend *backend)
{
  if ((backend != NULL) && (backend->poll != NULL)) {
    backend->poll(backend->ctx);
  }
}

void StorageBackend_Lock(const StorageBackend *backend)
{
  if ((backend != NULL) && (backend->lock != NULL)) {
    backend->lock(backend->ctx);
  }
}

void StorageBackend_Unlock(const StorageBackend *backend)
{
  if ((backend != NULL) && (backend->unlock != NULL)) {
    backend->unlock(backend->ctx);
  }
}

Storage_Status StorageBackend_CheckVoltage(const StorageBackend *backend)
{
  if ((backend != NULL) && (backend->voltage_ok != NULL) &&
      (backend->voltage_ok(backend->ctx) == 0)) {
    return STORAGE_ERR_VOLTAGE;
  }
  return STORAGE_OK;
}
