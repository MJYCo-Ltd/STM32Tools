#ifndef STM32TOOLS_DURABLE_RECORD_QUEUE_H
#define STM32TOOLS_DURABLE_RECORD_QUEUE_H

#include <Flash/DualBankStore.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed V1 NOR queue, not StorageLog: identity 0..11, opaque payload 12..51,
 * CRC at 52, commit at 56, application-confirmed ACK at 60. No MQTT semantics.
 * 64-byte records and 4-KiB erase sectors are explicit format requirements. */
#define DURABLE_RECORD_SIZE 64U
#define DURABLE_SECTOR_SLOTS 64U
#define DURABLE_INDEX_BYTES(count) (((count) + DURABLE_SECTOR_SLOTS - 1U) / DURABLE_SECTOR_SLOTS)
#define DURABLE_CACHE_COUNT 2U

typedef struct {
  uint32_t sector;
  uint8_t states[DURABLE_SECTOR_SLOTS / 4U];
  uint8_t valid;
} DurableSectorCache;

typedef struct {
  uint32_t data_offset, capacity, epoch_a, epoch_b, magic;
  uint8_t *sectors; /* caller owns DURABLE_INDEX_BYTES(capacity) bytes */
  void (*report_error)(Storage_Status status);
} DurableRecordQueueConfig;

typedef struct {
  const DurableRecordQueueConfig *config;
  const StoragePartitionMap *map;
  uint32_t epoch, sequence, write_slot, read_slot;
  uint32_t free_slots;
  DurableSectorCache cache[DURABLE_CACHE_COUNT];
  uint32_t ack_slot, ack_epoch, ack_sequence;
  uint8_t ack_pending, next_cache, ready;
  uint32_t partition;
  DualBankTransaction transaction;
} DurableRecordQueue;

/* config/map/summary storage and transaction.ctx must outlive the queue.
 * Init copies the transaction. begin/end are required as a pair: use no-op
 * callbacks only when the caller guarantees serialization. The same recursive
 * transaction protects the complete operation and nested epoch-bank access.
 * begin failure is propagated; end runs exactly once after each successful
 * begin. Init itself must not race other operations. No allocation, global
 * queue state, HAL/RTOS calls, product partition IDs or board dependencies.
 * Offsets are relative to the supplied partition. Caller supplies two distinct
 * epoch sectors, disjoint from the data region, in a writable NOR partition.
 */
Storage_Status DurableRecordQueue_InitEx(DurableRecordQueue *queue,
    const DurableRecordQueueConfig *config, const StoragePartitionMap *map,
    uint32_t partition, const DualBankTransaction *transaction);

/* Copies a complete 64-byte record; assigns identity/CRC/commit/ACK internally. */
Storage_Status DurableRecordQueue_Append(DurableRecordQueue *queue, const void *record);
Storage_Status DurableRecordQueue_Peek(DurableRecordQueue *queue, void *record);
/* Caller decides whether delivery has been confirmed. Exact identity is
 * required. An uncertain ACK remains pinned through cache eviction and cannot
 * be reclaimed before the caller verifies it. No pending record is overwritten. */
Storage_Status DurableRecordQueue_Acknowledge(DurableRecordQueue *queue,
                                             uint32_t epoch, uint32_t sequence);
/* Low-priority call: erase at most one safe sector; never an unconfirmed one. */
Storage_Status DurableRecordQueue_ReclaimOne(DurableRecordQueue *queue);

#ifdef __cplusplus
}
#endif
#endif
