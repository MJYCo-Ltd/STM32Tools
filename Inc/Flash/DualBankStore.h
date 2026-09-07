#ifndef STM32TOOLS_DUAL_BANK_STORE_H
#define STM32TOOLS_DUAL_BANK_STORE_H

#include <stdint.h>

#include <Flash/storage_partition.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t (*DualBankPayloadValidFn)(const void *payload);

/* Required paired callbacks, copied by Init. ctx must outlive the store.
 * begin failure is returned unchanged and must leave no lock held; end is
 * called only after successful begin. Use the same recursive lock as the
 * storage backend when it locks individual operations. The caller serializes
 * access to each store, including Init and its scratch buffers. */
typedef struct {
  void *ctx;
  Storage_Status (*begin)(void *ctx);
  void (*end)(void *ctx);
} DualBankTransaction;

typedef struct {
  DualBankTransaction transaction;
  const StoragePartitionMap *map;
  uint32_t partition;
  uint32_t bank_a_off;
  uint32_t bank_b_off;
  uint32_t bank_size;
  uint32_t payload_size;
  DualBankPayloadValidFn valid;
  uint32_t active_bank;
  uint32_t next_sequence;
  uint8_t have_snapshot;
} DualBankStore;

/**
 * scratch_a/scratch_b 需各能容纳 payload_size，仅 Init 期间使用。
 * 无有效快照时返回 STORAGE_ERR_NOT_FOUND（仍视为 Init 成功可后续 Save）。
 */
Storage_Status DualBankStore_Init(DualBankStore *store,
                                  const DualBankTransaction *transaction,
                                  const StoragePartitionMap *map,
                                  uint32_t partition, uint32_t bank_a_off,
                                  uint32_t bank_b_off, uint32_t bank_size,
                                  uint32_t payload_size,
                                  DualBankPayloadValidFn valid, void *scratch_a,
                                  void *scratch_b);

Storage_Status DualBankStore_Load(DualBankStore *store, void *payload_out);

Storage_Status DualBankStore_Save(DualBankStore *store, const void *payload);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_DUAL_BANK_STORE_H */
