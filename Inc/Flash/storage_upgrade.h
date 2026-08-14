#ifndef STM32TOOLS_STORAGE_UPGRADE_H
#define STM32TOOLS_STORAGE_UPGRADE_H

#include <stdint.h>

#include "Flash/storage_partition.h"
#include "Flash/storage_pack.h"
#include "Flash/storage_record.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  UPGRADE_STATE_IDLE = 0,
  UPGRADE_STATE_DOWNLOADING,
  UPGRADE_STATE_CANDIDATE_VALID,
  UPGRADE_STATE_BACKUP_VALID,
  UPGRADE_STATE_INSTALLING,
  UPGRADE_STATE_TRIAL_BOOT,
  UPGRADE_STATE_CONFIRMED,
  UPGRADE_STATE_ROLLBACK_PENDING,
  UPGRADE_STATE_ROLLING_BACK,
  UPGRADE_STATE_ROLLED_BACK,
  UPGRADE_STATE_FAILED
} UpgradeState;

STORAGE_PACK_BEGIN
typedef struct STORAGE_STRUCT_PACKED {
  uint32_t state;
  uint32_t active_version;
  uint32_t candidate_version;
  uint32_t candidate_length;
  uint32_t candidate_crc32;
  uint32_t trial_boot_count;
  uint32_t last_error;
  uint32_t reset_reason;
  uint32_t watchdog_resets;
  uint32_t phase_attempts;
} UpgradeStatePayload;
STORAGE_PACK_END

/**
 * Control partition layout (within STORAGE_PART_CONTROL):
 *  [0, 4KB)   meta bank A
 *  [4KB, 8KB) meta bank B
 *  [8KB, end) append-only upgrade state log
 */
typedef struct {
  const StoragePartitionMap *map;
  uint32_t partition;
  uint32_t partition_size;
  uint32_t next_sequence;
  UpgradeStatePayload latest;
  uint8_t has_latest;
} StorageUpgradeLog;

Storage_Status StorageUpgrade_Init(StorageUpgradeLog *log,
                                   const StoragePartitionMap *map,
                                   uint32_t partition,
                                   uint32_t partition_size);

Storage_Status StorageUpgrade_Get(const StorageUpgradeLog *log,
                                  UpgradeStatePayload *out);

Storage_Status StorageUpgrade_Append(StorageUpgradeLog *log,
                                     const UpgradeStatePayload *payload);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_STORAGE_UPGRADE_H */
