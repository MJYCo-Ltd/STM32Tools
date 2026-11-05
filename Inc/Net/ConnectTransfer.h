#ifndef STM32TOOLS_NET_CONNECT_TRANSFER_H
#define STM32TOOLS_NET_CONNECT_TRANSFER_H

#include <Net/ConnectTransport.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  CONNECT_TRANSFER_IDLE = 0, CONNECT_TRANSFER_RECEIVING,
  CONNECT_TRANSFER_VERIFIED, CONNECT_TRANSFER_CANCELED
} ConnectTransferState;

typedef enum {
  CONNECT_TRANSFER_OK = 0, CONNECT_TRANSFER_DUPLICATE,
  CONNECT_TRANSFER_INVALID, CONNECT_TRANSFER_OFFSET, CONNECT_TRANSFER_CONFLICT,
  CONNECT_TRANSFER_BUSY, CONNECT_TRANSFER_SINK_ERROR,
  CONNECT_TRANSFER_INCOMPLETE, CONNECT_TRANSFER_CRC
} ConnectTransferResult;

/* Caller-owned, no allocation or implicit persistence/resume. Identity and
 * timeout duration belong to the caller. received != durable != domain applied.
 * A canceled session retains diagnostics until Reset/Begin. */
typedef struct {
  uint32_t total, expected_crc, received, durable, running_crc, progress_tick;
  ConnectTransferState state;
} ConnectTransfer;

typedef struct {
  void *context;
  /* OK means the sink accepted ALL bytes; busy/error must accept no bytes.
   * A failed physical write with unknown outcome must be quarantined by its
   * owner, never retried on the strength of this RAM session. */
  ConnectTransferResult (*write)(void *context, uint32_t offset,
                                 const uint8_t *bytes, uint32_t length);
  /* Optional exact readback for buffered duplicates. Stream sinks omit it. */
  uint8_t (*matches)(void *context, uint32_t offset,
                     const uint8_t *bytes, uint32_t length);
} ConnectTransferSink;

void ConnectTransfer_Reset(ConnectTransfer *session);
ConnectTransferResult ConnectTransfer_Begin(ConnectTransfer *session,
    uint32_t total, uint32_t crc, uint32_t now);
void ConnectTransfer_Cancel(ConnectTransfer *session);
ConnectTransferResult ConnectTransfer_Accept(ConnectTransfer *session,
    const ConnectTransferSink *sink, uint32_t offset, const uint8_t *bytes,
    uint32_t length, uint32_t now);
ConnectTransferResult ConnectTransfer_AcceptBuffer(ConnectTransfer *session,
    uint8_t *buffer, uint32_t capacity, uint32_t offset, const uint8_t *bytes,
    uint32_t length, uint32_t now);
ConnectTransferResult ConnectTransfer_Complete(ConnectTransfer *session);
/* Same validation for every HTTP consumer; FAILED/release ownership stays in
 * the router/caller. Complete events must describe the exact final position. */
ConnectTransferResult ConnectTransfer_Event(ConnectTransfer *session,
    const ConnectTransferSink *sink, const ConnectDownloadEvent *event, uint32_t now);
ConnectTransferResult ConnectTransfer_ConfirmDurable(ConnectTransfer *session,
    uint32_t position);
uint8_t ConnectTransfer_TimedOut(const ConnectTransfer *session,
    uint32_t now, uint32_t timeout);
uint32_t ConnectTransfer_Crc(const ConnectTransfer *session);

#define CONNECT_TRANSFER_HEADER_SIZE 24U
#define CONNECT_TRANSFER_IDENTITY_MAX 39U
#define CONNECT_TRANSFER_BLOCK_MAX 192U
enum { CONNECT_TRANSFER_BEGIN=1, CONNECT_TRANSFER_DATA=2, CONNECT_TRANSFER_COMMIT=3 };
/* BTF1 framing only. kind and identity semantics are caller-owned. Spans borrow
 * input storage and expire with the synchronous dispatch, never retained. */
typedef struct {
  uint32_t total, offset, crc;
  const uint8_t *identity, *data;
  uint16_t length;
  uint8_t kind, operation, identity_length;
} ConnectTransferEnvelope;
uint8_t ConnectTransfer_Decode(const uint8_t *bytes, size_t length,
    ConnectTransferEnvelope *envelope);

#ifdef __cplusplus
}
#endif
#endif
