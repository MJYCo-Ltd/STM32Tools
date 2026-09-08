#include <Flash/SignedFirmware.h>
#include "monocypher.h"
#include "monocypher-ed25519.h"
#include <string.h>

static uint32_t Read32(const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8U) |
      ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U);
}

static uint8_t FieldEquals(const uint8_t *p, size_t size, const char *value)
{
  size_t n = strlen(value), i;
  if (n >= size || memcmp(p, value, n) != 0) return 0U;
  for (i = n; i < size; ++i) if (p[i] != 0U) return 0U;
  return 1U;
}

static void Poll(const SignedFirmwareReader *reader)
{
  if (reader->poll != NULL) reader->poll(reader->context);
}

Storage_Status SignedFirmware_VerifyImage(const SignedFirmwareReader *reader,
    const SignedFirmwarePolicy *policy, uint32_t image_length,
    uint32_t firmware_version)
{
  uint8_t footer[SIGNED_FIRMWARE_FOOTER_SIZE], chunk[512], digest[64];
  crypto_sha512_ctx hash;
  uint32_t body_length, offset;
  Storage_Status status;
  if (reader == NULL || reader->read == NULL || policy == NULL ||
      policy->magic == NULL || policy->public_key == NULL ||
      policy->product == NULL || policy->board == NULL || policy->hardware == NULL)
    return STORAGE_ERR_PARAM;
  if (image_length <= SIGNED_FIRMWARE_FOOTER_SIZE ||
      image_length > policy->image_capacity) return STORAGE_ERR_RANGE;
  body_length = image_length - SIGNED_FIRMWARE_FOOTER_SIZE;
  status = reader->read(reader->context, body_length, footer, sizeof(footer));
  if (status != STORAGE_OK) return status;
  if (memcmp(footer, policy->magic, 16U) != 0 || Read32(footer + 16) != 1U ||
      Read32(footer + 20) != firmware_version ||
      Read32(footer + 24) != body_length ||
      Read32(footer + 28) != policy->target_address ||
      !FieldEquals(footer + 32, 32U, policy->product) ||
      !FieldEquals(footer + 64, 16U, policy->board) ||
      !FieldEquals(footer + 80, 32U, policy->hardware)) return STORAGE_ERR_STATE;
  Poll(reader);
  if (crypto_ed25519_check(footer + 192, policy->public_key, footer, 192U) != 0)
    return STORAGE_ERR_CRC;
  crypto_sha512_init(&hash);
  for (offset = 0U; offset < body_length;) {
    uint32_t length = body_length - offset;
    if (length > sizeof(chunk)) length = sizeof(chunk);
    status = reader->read(reader->context, offset, chunk, length);
    if (status != STORAGE_OK) return status;
    crypto_sha512_update(&hash, chunk, length);
    offset += length;
    Poll(reader);
  }
  crypto_sha512_final(&hash, digest);
  return crypto_verify64(digest, footer + 112) == 0 ? STORAGE_OK : STORAGE_ERR_CRC;
}
