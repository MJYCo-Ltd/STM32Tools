#ifndef STM32TOOLS_STORAGE_RECORD_H
#define STM32TOOLS_STORAGE_RECORD_H

#include <stdint.h>

#include "Flash/storage_partition.h"
#include "Flash/storage_pack.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STORAGE_RECORD_MAGIC        0x52444353UL /* 'SCDR' */
#define STORAGE_RECORD_FORMAT_V1    1U
#define STORAGE_COMMIT_MARKER       0xA5C3E17FUL
#define STORAGE_ERASED_U32          0xFFFFFFFFUL

STORAGE_PACK_BEGIN
typedef struct STORAGE_STRUCT_PACKED {
  uint32_t magic;
  uint16_t format_version;
  uint16_t header_size;
  uint32_t sequence;
  uint32_t payload_length;
  uint32_t payload_crc32;
  uint32_t header_crc32;
  uint32_t commit_marker;
} StorageRecordHeader;
STORAGE_PACK_END

typedef struct {
  uint32_t sequence;
  uint32_t offset;
  uint32_t payload_length;
  uint8_t valid;
} StorageRecordLoc;

/** True if sequence a is strictly newer than b (unsigned wrap-safe). */
int Storage_SeqIsNewer(uint32_t a, uint32_t b);

Storage_Status StorageRecord_HeaderCrc(const StorageRecordHeader *header,
                                       uint32_t *crc_out);

Storage_Status StorageRecord_Append(
    const StoragePartitionMap *map, uint32_t partition, uint32_t region_offset,
    uint32_t region_size, uint32_t sequence, const void *payload,
    uint32_t payload_length, uint32_t *written_offset);

/** Write a record at an exact erased offset (no scan). */
Storage_Status StorageRecord_WriteAt(
    const StoragePartitionMap *map, uint32_t partition, uint32_t offset,
    uint32_t max_end, uint32_t sequence, const void *payload,
    uint32_t payload_length);

Storage_Status StorageRecord_FindLatest(
    const StoragePartitionMap *map, uint32_t partition, uint32_t region_offset,
    uint32_t region_size, StorageRecordLoc *out_loc, void *payload_buf,
    uint32_t payload_buf_size);

Storage_Status StorageRecord_ReadPayload(
    const StoragePartitionMap *map, uint32_t partition, uint32_t record_offset,
    void *payload_buf, uint32_t payload_buf_size, uint32_t *payload_length);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_STORAGE_RECORD_H */
