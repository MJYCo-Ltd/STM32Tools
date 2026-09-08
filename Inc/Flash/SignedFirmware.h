#ifndef STM32TOOLS_SIGNED_FIRMWARE_H
#define STM32TOOLS_SIGNED_FIRMWARE_H

#include <stdint.h>
#include <Flash/storage_backend.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIGNED_FIRMWARE_FOOTER_SIZE 256U

/* All pointers are borrowed for the synchronous verification call.
 * The caller owns flash serialization and the decision to install an image.
 * read() addresses the selected image, starting at offset zero; poll() is
 * optional and may feed a watchdog/yield without changing the image. */
typedef struct {
  void *context;
  Storage_Status (*read)(void *context, uint32_t offset, void *data, uint32_t length);
  void (*poll)(void *context);
} SignedFirmwareReader;

typedef struct {
  const uint8_t *magic;       /* Exactly 16 bytes, including padding. */
  const uint8_t *public_key;  /* Ed25519 public key, exactly 32 bytes. */
  const char *product;       /* NUL-terminated; encoded field is 32 bytes. */
  const char *board;         /* Encoded field is 16 bytes. */
  const char *hardware;      /* Encoded field is 32 bytes. */
  uint32_t target_address;
  uint32_t image_capacity;   /* Includes the 256-byte footer. */
} SignedFirmwarePolicy;

/* V1 footer: magic[16], LE32 schema/version/body size/target, product[32],
 * board[16], hardware[32], SHA-512[64], reserved[16], Ed25519 signature[64].
 * Signature authenticates footer bytes 0..191; all body bytes are hashed.
 * Identity, key and storage location are supplied by the application.
 * This verifies authenticity/integrity; it is not an anti-rollback policy. */
Storage_Status SignedFirmware_VerifyImage(const SignedFirmwareReader *reader,
    const SignedFirmwarePolicy *policy, uint32_t image_length,
    uint32_t firmware_version);

#ifdef __cplusplus
}
#endif
#endif
