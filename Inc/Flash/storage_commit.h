#ifndef STM32TOOLS_STORAGE_COMMIT_H
#define STM32TOOLS_STORAGE_COMMIT_H

#include <stddef.h>
#include <stdint.h>

#include "Flash/storage_partition.h"
#include "Flash/storage_record.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CRC32 over [0, total_size - 4) with the u32 at crc_field_offset as 0.
 * Does not modify object.
 */
uint32_t Storage_CrcExcludingCommit(const void *object, size_t total_size,
                                    size_t crc_field_offset);

/**
 * Compute CRC32 over [0, total_size - 4) with the u32 at crc_field_offset
 * temporarily treated as 0 and the trailing commit word ignored.
 * Writes the CRC into crc_field_offset and sets the last u32 to ERASED.
 */
Storage_Status Storage_PrepareCommitObject(void *object, size_t total_size,
                                           size_t crc_field_offset);

/**
 * Write object without commit, read-back verify, then write commit marker.
 * Object must already be prepared (CRC filled, commit == ERASED).
 */
Storage_Status Storage_WriteCommitted(const StoragePartitionMap *map,
                                      uint32_t partition, uint32_t offset,
                                      const void *object, size_t total_size);

/** Prepare + WriteCommitted. */
Storage_Status Storage_CommitObject(const StoragePartitionMap *map,
                                    uint32_t partition, uint32_t offset,
                                    void *object, size_t total_size,
                                    size_t crc_field_offset);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_STORAGE_COMMIT_H */
