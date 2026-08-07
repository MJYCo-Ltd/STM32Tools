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
