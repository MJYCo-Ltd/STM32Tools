/* ML307 HTTP cached/range command helpers and bounded frame parsers. */
#ifndef STM32TOOLS_ML307_HTTP_H
#define STM32TOOLS_ML307_HTTP_H

#include <stddef.h>
#include <stdint.h>
#include <ML307/ml307.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t http_id;
  uint16_t status_code;
  uint32_t header_length;
  uint32_t content_length;
} ML307_HttpRecvEvent;

typedef enum {
  ML307_HTTP_URC_NONE = 0,
  ML307_HTTP_URC_RECV,
  ML307_HTTP_URC_ERR
} ML307_HttpUrcKind;

typedef struct {
  ML307_HttpUrcKind kind;
  uint8_t http_id;
  uint16_t status_code;
  uint32_t header_length;
  uint32_t content_length;
  uint32_t error_code;
} ML307_HttpUrcEvent;

/* Runtime limits belong to the caller, not a board-specific global. */
typedef struct {
  uint32_t maximum_payload_length;
  size_t urc_line_capacity; /* complete interleaved URC must be shorter than this */
} ML307_HttpReadLimits;

typedef struct {
  size_t data_length;
  size_t consumed;
  uint32_t unread_length;
  uint32_t module_error;
  uint8_t has_module_error; /* err=0 is distinct from no error event */
} ML307_HttpReadResult;

ML307_Result ML307_HttpBuildCreate(char *output, size_t output_size,
                                   const char *origin, size_t *length);
ML307_Result ML307_HttpBuildCached(char *output, size_t output_size,
                                   uint8_t http_id, uint8_t enabled,
                                   size_t *length);
ML307_Result ML307_HttpBuildSsl(char *output, size_t output_size,
                               uint8_t http_id, uint8_t enabled,
                               uint8_t ssl_id, size_t *length);
ML307_Result ML307_HttpBuildHeader(char *output, size_t output_size,
                                   uint8_t http_id, const char *header,
                                   size_t *length);
ML307_Result ML307_HttpBuildGet(char *output, size_t output_size,
                                uint8_t http_id, const char *path,
                                size_t *length);
ML307_Result ML307_HttpBuildRead(char *output, size_t output_size,
                                 uint8_t http_id, uint8_t data_type,
                                 uint16_t read_length, size_t *length);
ML307_Result ML307_HttpBuildDelete(char *output, size_t output_size,
                                   uint8_t http_id, size_t *length);
ML307_Result ML307_HttpParseCreate(const char *response, uint8_t *http_id);

/* One complete ML307C recv/err line; length excludes the C terminator.
 * Optional CR, LF or CRLF only; no embedded NUL, signs, overflow or junk.
 * Unsupported kinds return NOT_FOUND. No hardware or transport side effects. */
ML307_Result ML307_HttpParseUrcLine(const uint8_t *line, size_t length,
                                   ML307_HttpUrcEvent *event);
/* Compatibility string API, delegated to the same strict line grammar. */
ML307_Result ML307_HttpParseRecvUrc(const char *line,
                                    ML307_HttpRecvEvent *event);

/* Parse ONE bounded MHTTPREAD frame. The declared body is opaque even when
 * it contains NUL, CR/LF, ERROR, OK or a URC prefix. Only complete lines after
 * the body may contain interleaved recv/err URCs; an err for expected_http_id
 * fails with raw module error evidence in result (no product error mapping).
 * Result is cleared first; data_length/unread_length/consumed are set only on
 * success, has_module_error may be set on failure. The data buffer is never
 * changed on incomplete/invalid input. NOT_FOUND means incomplete/no frame.
 * Reentrant: no parser globals, no HAL clock, no UART/OTA ownership state.
 */
ML307_Result ML307_HttpParseReadFrame(const uint8_t *response,
    size_t response_length, uint8_t expected_http_id, uint8_t expected_data_type,
    const ML307_HttpReadLimits *limits, uint8_t *data, size_t data_capacity,
    ML307_HttpReadResult *result);

/* Source-compatible wrapper. Delegates to ReadFrame with protocol-wide limits
 * (not a board's download window); malformed tails now return INVALID_VALUE.
 * Binary data may contain NUL/CR/LF/comma. */
ML307_Result ML307_HttpParseRead(const uint8_t *response,
                                 size_t response_length,
                                 uint8_t expected_http_id,
                                 uint8_t expected_data_type, uint8_t *data,
                                 size_t data_capacity, size_t *data_length,
                                 uint32_t *unread_length);

#ifdef __cplusplus
}
#endif
#endif
