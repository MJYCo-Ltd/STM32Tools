#include <Flash/DualBankStore.h>

#include <Flash/storage_bank.h>
#include <Flash/storage_record.h>

#include <string.h>

static Storage_Status DualBankStore_FindBank(const DualBankStore *store,
                                             uint32_t offset,
                                             StorageRecordLoc *location,
                                             void *payload)
{
  return StorageRecord_FindLatest(store->map, store->partition, offset,
                                  store->bank_size, location, payload,
                                  store->payload_size);
}

Storage_Status DualBankStore_Init(DualBankStore *store,
                                  const DualBankTransaction *transaction,
                                  const StoragePartitionMap *map,
                                  uint32_t partition, uint32_t bank_a_off,
                                  uint32_t bank_b_off, uint32_t bank_size,
                                  uint32_t payload_size,
                                  DualBankPayloadValidFn valid, void *scratch_a,
                                  void *scratch_b)
{
  StorageRecordLoc location_a;
  StorageRecordLoc location_b;
  Storage_Status status_a;
  Storage_Status status_b;
  Storage_Status lock_status;

  if ((store == NULL) || (map == NULL) || (valid == NULL) ||
      (scratch_a == NULL) || (scratch_b == NULL) || (payload_size == 0U) ||
      (bank_size == 0U) || (transaction == NULL) ||
      (transaction->begin == NULL) || (transaction->end == NULL)) {
    return STORAGE_ERR_PARAM;
  }

  memset(store, 0, sizeof(*store));
  store->transaction = *transaction;
  store->map = map;
  store->partition = partition;
  store->bank_a_off = bank_a_off;
  store->bank_b_off = bank_b_off;
  store->bank_size = bank_size;
  store->payload_size = payload_size;
  store->valid = valid;
  store->active_bank = 0U;
  store->next_sequence = 1U;
  store->have_snapshot = 0U;

  lock_status = store->transaction.begin(store->transaction.ctx);
  if (lock_status != STORAGE_OK) {
    return lock_status;
  }
  status_a = DualBankStore_FindBank(store, bank_a_off, &location_a, scratch_a);
  status_b = DualBankStore_FindBank(store, bank_b_off, &location_b, scratch_b);
  store->transaction.end(store->transaction.ctx);

  if ((status_a != STORAGE_OK) && (status_a != STORAGE_ERR_NOT_FOUND)) {
    return status_a;
  }
  if ((status_b != STORAGE_OK) && (status_b != STORAGE_ERR_NOT_FOUND)) {
    return status_b;
  }
  if ((status_a == STORAGE_OK) && (valid(scratch_a) == 0U)) {
    return STORAGE_ERR_STATE;
  }
  if ((status_b == STORAGE_OK) && (valid(scratch_b) == 0U)) {
    return STORAGE_ERR_STATE;
  }

  if ((status_a == STORAGE_OK) && (status_b == STORAGE_OK)) {
    if (Storage_SeqIsNewer(location_b.sequence, location_a.sequence) != 0) {
      store->active_bank = 1U;
      store->next_sequence = location_b.sequence + 1U;
    } else {
      store->next_sequence = location_a.sequence + 1U;
    }
    store->have_snapshot = 1U;
  } else if (status_a == STORAGE_OK) {
    store->next_sequence = location_a.sequence + 1U;
    store->have_snapshot = 1U;
  } else if (status_b == STORAGE_OK) {
    store->active_bank = 1U;
    store->next_sequence = location_b.sequence + 1U;
    store->have_snapshot = 1U;
  }

  return (store->have_snapshot != 0U) ? STORAGE_OK : STORAGE_ERR_NOT_FOUND;
}

Storage_Status DualBankStore_Load(DualBankStore *store, void *payload_out)
{
  StorageRecordLoc location;
  Storage_Status status;
  uint32_t offset;

  if ((store == NULL) || (payload_out == NULL) || (store->map == NULL) ||
      (store->valid == NULL) || (store->transaction.begin == NULL) ||
      (store->transaction.end == NULL)) {
    return STORAGE_ERR_PARAM;
  }
  if (store->have_snapshot == 0U) {
    return STORAGE_ERR_NOT_FOUND;
  }
  offset = (store->active_bank == 0U) ? store->bank_a_off : store->bank_b_off;
  status = store->transaction.begin(store->transaction.ctx);
  if (status != STORAGE_OK) {
    return status;
  }
  status = DualBankStore_FindBank(store, offset, &location, payload_out);
  store->transaction.end(store->transaction.ctx);
  if ((status != STORAGE_OK) || (store->valid(payload_out) == 0U)) {
    return (status != STORAGE_OK) ? status : STORAGE_ERR_STATE;
  }
  return STORAGE_OK;
}

Storage_Status DualBankStore_Save(DualBankStore *store, const void *payload)
{
  Storage_Status status;
  uint32_t active_offset;
  uint32_t inactive_offset;
  uint32_t written_offset;

  if ((store == NULL) || (payload == NULL) || (store->map == NULL) ||
      (store->transaction.begin == NULL) || (store->transaction.end == NULL)) {
    return STORAGE_ERR_PARAM;
  }
  active_offset =
      (store->active_bank == 0U) ? store->bank_a_off : store->bank_b_off;
  inactive_offset =
      (store->active_bank == 0U) ? store->bank_b_off : store->bank_a_off;
  status = store->transaction.begin(store->transaction.ctx);
  if (status != STORAGE_OK) {
    return status;
  }
  status = StorageRecord_Append(
      store->map, store->partition, active_offset, store->bank_size,
      store->next_sequence, payload, store->payload_size, &written_offset);
  if (status == STORAGE_ERR_NO_SPACE) {
    status = StorageBank_Switch(
        store->map, store->partition, inactive_offset, store->bank_size,
        active_offset, store->bank_size, store->next_sequence, payload,
        store->payload_size, 0U, 0U);
    if (status == STORAGE_OK) {
      store->active_bank = (store->active_bank == 0U) ? 1U : 0U;
    }
  }
  store->transaction.end(store->transaction.ctx);
  if (status == STORAGE_OK) {
    ++store->next_sequence;
    store->have_snapshot = 1U;
  }
  return status;
}
