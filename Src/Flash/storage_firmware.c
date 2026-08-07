#include "Flash/storage_firmware.h"

#include "Flash/nor_flash.h"
#include "Flash/storage_commit.h"
#include "Flash/storage_crc.h"
#include "Flash/storage_record.h"

#include <stddef.h>
#include <string.h>

uint32_t StorageFirmware_ImageCapacity(uint32_t slot_size)
{
  if (slot_size <= STORAGE_FW_MANIFEST_SIZE) {
    return 0U;
  }
  return slot_size - STORAGE_FW_MANIFEST_SIZE;
}

static uint32_t ManifestHeaderCrc(const StorageFirmwareManifest *m)
{
  if (m == NULL) {
    return 0U;
  }
  return Storage_CrcExcludingCommit(
      m, sizeof(*m), offsetof(StorageFirmwareManifest, header_crc32));
}

Storage_Status StorageFirmware_InitSlot(StorageFirmwareSlot *slot,
                                        const StoragePartitionMap *map,
                                        uint32_t partition,
                                        uint32_t slot_size)
{
  if ((slot == NULL) || (map == NULL) || (slot_size < STORAGE_FW_MANIFEST_SIZE) ||
      ((slot_size % NOR_FLASH_SECTOR_SIZE) != 0U)) {
    return STORAGE_ERR_PARAM;
  }
  memset(slot, 0, sizeof(*slot));
  slot->map = map;
  slot->partition = partition;
  slot->slot_size = slot_size;
  return STORAGE_OK;
}

Storage_Status StorageFirmware_BeginWrite(StorageFirmwareSlot *slot,
                                          uint32_t image_length)
{
  uint32_t cap;
  uint32_t offset;
  Storage_Status st;

  if (slot == NULL) {
    return STORAGE_ERR_PARAM;
  }
  cap = StorageFirmware_ImageCapacity(slot->slot_size);
  if ((image_length == 0U) || (image_length > cap)) {
    return STORAGE_ERR_RANGE;
  }
  /* Erase whole slot including old manifest. */
  for (offset = 0U; offset < slot->slot_size; offset += NOR_FLASH_SECTOR_SIZE) {
    st = Storage_EraseSector(slot->map, slot->partition, offset);
    if (st != STORAGE_OK) {
      return st;
    }
  }
  slot->write_offset = 0U;
  slot->expected_length = image_length;
  slot->running_crc = 0xFFFFFFFFUL;
  slot->writing = 1U;
  return STORAGE_OK;
}

Storage_Status StorageFirmware_WriteChunk(StorageFirmwareSlot *slot,
                                          const void *data, uint32_t length)
{
  Storage_Status st;

  if ((slot == NULL) || (slot->writing == 0U) || (data == NULL) ||
      (length == 0U)) {
    return STORAGE_ERR_PARAM;
  }
  if ((slot->write_offset + length) < slot->write_offset ||
      ((slot->write_offset + length) > slot->expected_length)) {
    return STORAGE_ERR_RANGE;
  }
  st = Storage_Write(slot->map, slot->partition,
                     STORAGE_FW_MANIFEST_SIZE + slot->write_offset, data,
                     length);
  if (st != STORAGE_OK) {
    return st;
  }
  slot->running_crc = Storage_Crc32Update(slot->running_crc, data, length);
  slot->write_offset += length;
  return STORAGE_OK;
}

Storage_Status StorageFirmware_Finish(StorageFirmwareSlot *slot,
                                      const StorageFirmwareManifest *meta_in,
                                      StorageFirmwareManifest *meta_out)
{
  StorageFirmwareManifest manifest;
  uint32_t image_crc;
  Storage_Status st;

  if ((slot == NULL) || (meta_in == NULL) || (slot->writing == 0U)) {
    return STORAGE_ERR_PARAM;
  }
  if (slot->write_offset != slot->expected_length) {
    return STORAGE_ERR_STATE;
  }
  image_crc = slot->running_crc ^ 0xFFFFFFFFUL;
  if (meta_in->image_crc32 != image_crc) {
    return STORAGE_ERR_CRC;
  }
  if ((meta_in->image_length != slot->expected_length) ||
      (meta_in->image_length >
       StorageFirmware_ImageCapacity(slot->slot_size))) {
    return STORAGE_ERR_RANGE;
  }

  memset(&manifest, 0, sizeof(manifest));
  manifest = *meta_in;
  manifest.magic = STORAGE_FW_MANIFEST_MAGIC;
  manifest.manifest_version = STORAGE_FW_MANIFEST_VERSION;
  manifest.image_length = slot->expected_length;
  manifest.image_crc32 = image_crc;

  st = Storage_CommitObject(slot->map, slot->partition, 0U, &manifest,
                            sizeof(manifest),
                            offsetof(StorageFirmwareManifest, header_crc32));
  if (st != STORAGE_OK) {
    return st;
  }
  slot->writing = 0U;
  if (meta_out != NULL) {
    *meta_out = manifest;
  }
  return STORAGE_OK;
}

Storage_Status StorageFirmware_Invalidate(StorageFirmwareSlot *slot)
{
  if (slot == NULL) {
    return STORAGE_ERR_PARAM;
  }
  slot->writing = 0U;
  return Storage_EraseSector(slot->map, slot->partition, 0U);
}

Storage_Status StorageFirmware_ReadManifest(StorageFirmwareSlot *slot,
                                            StorageFirmwareManifest *manifest)
{
  Storage_Status st;

  if ((slot == NULL) || (manifest == NULL)) {
    return STORAGE_ERR_PARAM;
  }
  st = Storage_Read(slot->map, slot->partition, 0U, manifest, sizeof(*manifest));
  if (st != STORAGE_OK) {
    return st;
  }
  if ((manifest->magic != STORAGE_FW_MANIFEST_MAGIC) ||
      (manifest->commit_marker != STORAGE_COMMIT_MARKER) ||
      (ManifestHeaderCrc(manifest) != manifest->header_crc32)) {
    return STORAGE_ERR_CRC;
  }
  return STORAGE_OK;
}

Storage_Status StorageFirmware_IsValid(StorageFirmwareSlot *slot,
                                       StorageFirmwareManifest *manifest)
{
  StorageFirmwareManifest local;
  StorageFirmwareManifest *m = (manifest != NULL) ? manifest : &local;
  Storage_Status st;
  uint8_t chunk[128];
  uint32_t left;
  uint32_t pos;
  uint32_t crc;

  st = StorageFirmware_ReadManifest(slot, m);
  if (st != STORAGE_OK) {
    return st;
  }
  if ((m->image_length == 0U) ||
      (m->image_length > StorageFirmware_ImageCapacity(slot->slot_size))) {
    return STORAGE_ERR_RANGE;
  }
  left = m->image_length;
  pos = STORAGE_FW_MANIFEST_SIZE;
  crc = 0xFFFFFFFFUL;
  while (left > 0U) {
    uint32_t n = (left > sizeof(chunk)) ? (uint32_t)sizeof(chunk) : left;
    st = Storage_Read(slot->map, slot->partition, pos, chunk, n);
    if (st != STORAGE_OK) {
      return st;
    }
    crc = Storage_Crc32Update(crc, chunk, n);
    pos += n;
    left -= n;
  }
  if ((crc ^ 0xFFFFFFFFUL) != m->image_crc32) {
    return STORAGE_ERR_CRC;
  }
  return STORAGE_OK;
}

Storage_Status StorageFirmware_ReadImage(StorageFirmwareSlot *slot,
                                         uint32_t offset, void *buffer,
                                         uint32_t length)
{
  StorageFirmwareManifest m;
  Storage_Status st;

  st = StorageFirmware_ReadManifest(slot, &m);
  if (st != STORAGE_OK) {
    return st;
  }
  if (NorFlash_CheckRange(m.image_length, offset, length) != NOR_FLASH_OK) {
    return STORAGE_ERR_RANGE;
  }
  return Storage_Read(slot->map, slot->partition,
                      STORAGE_FW_MANIFEST_SIZE + offset, buffer, length);
}
