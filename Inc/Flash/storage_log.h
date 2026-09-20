#ifndef STM32TOOLS_STORAGE_LOG_H
#define STM32TOOLS_STORAGE_LOG_H

#include <stdint.h>

#include "Flash/storage_partition.h"
#include "Flash/storage_pack.h"
#include "Flash/storage_record.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STORAGE_LOG_SECTOR_MAGIC 0x534C4F47UL /* 'SLOG' */

STORAGE_PACK_BEGIN
typedef struct STORAGE_STRUCT_PACKED {
  uint32_t magic;
  uint32_t sector_sequence;
  uint32_t erase_count;
  uint32_t header_crc32;
  uint32_t commit_marker;
} StorageLogSectorHeader;
STORAGE_PACK_END

typedef struct {
  const StoragePartitionMap *map;
  uint32_t partition;
  uint32_t region_offset;
  uint32_t region_size;
  uint32_t next_sequence;
  uint32_t active_sector_index;
  uint32_t write_offset_in_sector;
} StorageLog;

/* Bounded streaming reader. Zero-initialize a cursor to start/restart a pass.
 * A receipt identifies a record AND its sector generation, not just an offset.
 * StorageLog APIs require caller serialization (e.g. the product Flash mutex). */
typedef struct {
  uint32_t sector, offset, remaining;
  uint8_t started;
} StorageLogCursor;
typedef struct {
  StorageRecordLoc record;
  uint32_t sector_sequence, payload_crc32;
} StorageLogReceipt;

/* BUSY means the bounded scan made progress; call again later. NOT_FOUND ends
 * a pass. A failed payload read does not advance past the pending record. */
Storage_Status StorageLog_NextPending(StorageLog *log, StorageLogCursor *cursor,
    StorageLogReceipt *receipt, void *payload, uint32_t capacity,
    uint32_t *length);
/* After successful delivery only: clear the commit word to a durable tombstone.
 * No sector erase or record relocation; payload/header layout stays V1.
 * All shared readers/writers (including a Bootloader) must understand ACK
 * tombstones before enabling this API. Legacy pending records remain readable.
 * Idempotent for the same receipt; refuses stale locations after ring reuse. */
Storage_Status StorageLog_Acknowledge(StorageLog *log,
    const StorageLogReceipt *receipt);

Storage_Status StorageLog_Init(StorageLog *log, const StoragePartitionMap *map,
                               uint32_t partition, uint32_t region_offset,
                               uint32_t region_size);

Storage_Status StorageLog_Append(StorageLog *log, const void *payload,
                                 uint32_t payload_length);

Storage_Status StorageLog_GetRecent(StorageLog *log, uint32_t max_count,
                                    StorageRecordLoc *out_locs,
                                    uint32_t *out_count);

Storage_Status StorageLog_Clear(StorageLog *log);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_STORAGE_LOG_H */
