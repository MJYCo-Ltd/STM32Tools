#ifndef STM32TOOLS_STORAGE_BANK_H
#define STM32TOOLS_STORAGE_BANK_H

#include <stdint.h>

#include "Flash/storage_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t (*StorageEraseAllowedFn)(void);

/** Verify one blank sector, erasing it only if dirty. An optional admission
 * check runs just before erase. The caller owns the entire transaction and
 * is responsible for proving that the sector is disposable. */
Storage_Status StorageBank_PrepareSector(const StoragePartitionMap *map,
                                        uint32_t partition, uint32_t offset,
                                        StorageEraseAllowedFn allowed);

/** Erase [region_offset, region_offset + region_size) in sector steps. */
Storage_Status StorageBank_EraseRegion(const StoragePartitionMap *map,
                                       uint32_t partition,
                                       uint32_t region_offset,
                                       uint32_t region_size);

/**
 * Dual-bank / compact helper:
 * 1) erase dest bank
 * 2) write committed snapshot record at dest
 * 3) erase old bank
 * 4) optionally erase extra region (e.g. append log)
 */
Storage_Status StorageBank_Switch(const StoragePartitionMap *map,
                                  uint32_t partition, uint32_t dest_offset,
                                  uint32_t dest_size, uint32_t old_offset,
                                  uint32_t old_size, uint32_t sequence,
                                  const void *payload, uint32_t payload_length,
                                  uint32_t extra_erase_offset,
                                  uint32_t extra_erase_size);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_STORAGE_BANK_H */
