#include "Flash/storage_upgrade.h"

#include "Flash/nor_flash.h"
#include "Flash/storage_bank.h"

#include <string.h>

#define UPGRADE_META_A_OFF 0U
#define UPGRADE_META_B_OFF NOR_FLASH_SECTOR_SIZE
#define UPGRADE_LOG_OFF    (NOR_FLASH_SECTOR_SIZE * 2U)

static Storage_Status LoadLatest(StorageUpgradeLog *log)
{
  StorageRecordLoc loc_a;
  StorageRecordLoc loc_b;
  StorageRecordLoc loc_log;
  UpgradeStatePayload payload;
  Storage_Status st;
  uint8_t have = 0U;
  uint32_t best_seq = 0U;
  UpgradeStatePayload best;

  memset(&best, 0, sizeof(best));
  memset(&log->latest, 0, sizeof(log->latest));
  log->has_latest = 0U;
  log->next_sequence = 1U;

  st = StorageRecord_FindLatest(log->map, log->partition, UPGRADE_META_A_OFF,
                                NOR_FLASH_SECTOR_SIZE, &loc_a, &payload,
                                sizeof(payload));
  if (st == STORAGE_OK) {
    best = payload;
    best_seq = loc_a.sequence;
    have = 1U;
  } else if (st != STORAGE_ERR_NOT_FOUND) {
    return st;
  }
  st = StorageRecord_FindLatest(log->map, log->partition, UPGRADE_META_B_OFF,
                                NOR_FLASH_SECTOR_SIZE, &loc_b, &payload,
                                sizeof(payload));
  if (st == STORAGE_OK) {
    if ((have == 0U) || (Storage_SeqIsNewer(loc_b.sequence, best_seq) != 0)) {
      best = payload;
      best_seq = loc_b.sequence;
      have = 1U;
    }
  } else if (st != STORAGE_ERR_NOT_FOUND) {
    return st;
  }
  if (log->partition_size > UPGRADE_LOG_OFF) {
    st = StorageRecord_FindLatest(
        log->map, log->partition, UPGRADE_LOG_OFF,
        log->partition_size - UPGRADE_LOG_OFF, &loc_log, &payload,
        sizeof(payload));
    if (st == STORAGE_OK) {
      if ((have == 0U) ||
          (Storage_SeqIsNewer(loc_log.sequence, best_seq) != 0)) {
        best = payload;
        best_seq = loc_log.sequence;
        have = 1U;
      }
    } else if (st != STORAGE_ERR_NOT_FOUND) {
      return st;
    }
  }

  if (have != 0U) {
    log->latest = best;
    log->has_latest = 1U;
    log->next_sequence = best_seq + 1U;
  }
  return STORAGE_OK;
}

static Storage_Status Compact(StorageUpgradeLog *log)
{
  StorageRecordLoc loc_a;
  StorageRecordLoc loc_b;
  uint32_t active_is_a = 1U;
  uint32_t dest_off;
  uint32_t erase_off;
  Storage_Status st;
  uint32_t extra_size;

  st = StorageRecord_FindLatest(log->map, log->partition, UPGRADE_META_A_OFF,
                                NOR_FLASH_SECTOR_SIZE, &loc_a, NULL, 0U);
  if (st == STORAGE_OK) {
    Storage_Status stb = StorageRecord_FindLatest(
        log->map, log->partition, UPGRADE_META_B_OFF, NOR_FLASH_SECTOR_SIZE,
        &loc_b, NULL, 0U);
    if ((stb == STORAGE_OK) &&
        (Storage_SeqIsNewer(loc_b.sequence, loc_a.sequence) != 0)) {
      active_is_a = 0U;
    }
  } else {
    active_is_a = 0U;
  }

  dest_off = (active_is_a != 0U) ? UPGRADE_META_B_OFF : UPGRADE_META_A_OFF;
  erase_off = (active_is_a != 0U) ? UPGRADE_META_A_OFF : UPGRADE_META_B_OFF;
  extra_size = (log->partition_size > UPGRADE_LOG_OFF)
                   ? (log->partition_size - UPGRADE_LOG_OFF)
                   : 0U;

  st = StorageBank_Switch(log->map, log->partition, dest_off,
                          NOR_FLASH_SECTOR_SIZE, erase_off,
                          NOR_FLASH_SECTOR_SIZE, log->next_sequence,
                          &log->latest, (uint32_t)sizeof(log->latest),
                          UPGRADE_LOG_OFF, extra_size);
  if (st != STORAGE_OK) {
    return st;
  }
  log->next_sequence += 1U;
  return STORAGE_OK;
}

Storage_Status StorageUpgrade_Init(StorageUpgradeLog *log,
                                   const StoragePartitionMap *map,
                                   uint32_t partition,
                                   uint32_t partition_size)
{
  if ((log == NULL) || (map == NULL) ||
      (partition_size < (NOR_FLASH_SECTOR_SIZE * 2U)) ||
      ((partition_size % NOR_FLASH_SECTOR_SIZE) != 0U)) {
    return STORAGE_ERR_PARAM;
  }
  memset(log, 0, sizeof(*log));
  log->map = map;
  log->partition = partition;
  log->partition_size = partition_size;
  return LoadLatest(log);
}

Storage_Status StorageUpgrade_Get(const StorageUpgradeLog *log,
                                  UpgradeStatePayload *out)
{
  if ((log == NULL) || (out == NULL)) {
    return STORAGE_ERR_PARAM;
  }
  if (log->has_latest == 0U) {
    memset(out, 0, sizeof(*out));
    out->state = (uint32_t)UPGRADE_STATE_IDLE;
    return STORAGE_ERR_NOT_FOUND;
  }
  *out = log->latest;
  return STORAGE_OK;
}

Storage_Status StorageUpgrade_Append(StorageUpgradeLog *log,
                                     const UpgradeStatePayload *payload)
{
  Storage_Status st;
  uint32_t written = 0U;

  if ((log == NULL) || (payload == NULL)) {
    return STORAGE_ERR_PARAM;
  }
  if (log->partition_size <= UPGRADE_LOG_OFF) {
    return STORAGE_ERR_NO_SPACE;
  }

  st = StorageRecord_Append(log->map, log->partition, UPGRADE_LOG_OFF,
                            log->partition_size - UPGRADE_LOG_OFF,
                            log->next_sequence, payload,
                            (uint32_t)sizeof(*payload), &written);
  if (st == STORAGE_ERR_NO_SPACE) {
    log->latest = *payload;
    log->has_latest = 1U;
    st = Compact(log);
    if (st != STORAGE_OK) {
      return st;
    }
    st = StorageRecord_Append(log->map, log->partition, UPGRADE_LOG_OFF,
                              log->partition_size - UPGRADE_LOG_OFF,
                              log->next_sequence, payload,
                              (uint32_t)sizeof(*payload), &written);
  }
  if (st != STORAGE_OK) {
    return st;
  }
  log->latest = *payload;
  log->has_latest = 1U;
  log->next_sequence += 1U;
  return STORAGE_OK;
}
