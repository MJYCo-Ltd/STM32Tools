#ifndef STM32TOOLS_HTTP_RANGE_H
#define STM32TOOLS_HTTP_RANGE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define HTTP_RANGE_HOST_SIZE 96U
#define HTTP_RANGE_PATH_SIZE 192U
#define HTTP_RANGE_ORIGIN_SIZE 112U
typedef struct {
  char host[HTTP_RANGE_HOST_SIZE];
  char path[HTTP_RANGE_PATH_SIZE];
  char origin[HTTP_RANGE_ORIGIN_SIZE];
  uint16_t port;
  uint8_t https;
} HttpRangeUrl;
typedef struct {
  uint32_t content_length, range_start, range_end, total_length;
} HttpRangeHeaders;
/* Optional visitor for nonstandard headers. Spans are not NUL terminated.
 * Return zero to reject a header, one to accept/ignore it. */
typedef uint8_t (*HttpRangeHeaderFn)(void *ctx, const uint8_t *name,
    size_t name_length, const uint8_t *value, size_t value_length);
uint8_t HttpRange_ParseUrl(const char *url, HttpRangeUrl *parsed);
uint8_t HttpRange_BuildGet(char *output, size_t capacity, const char *path,
    const char *host, uint32_t range_start, uint32_t range_end);
/* Fixed-length single-range responses only. Rejects duplicate length/range
 * and Transfer-Encoding. No checksum policy or bearer/OTA state is imposed. */
uint8_t HttpRange_ParseHeader(const uint8_t *header, size_t length,
    uint32_t expected_start, uint32_t expected_length, uint32_t expected_total,
    HttpRangeHeaders *metadata, HttpRangeHeaderFn extra, void *ctx);
/* 1 complete, 0 incomplete, -1 invalid; outputs valid only on success.
 * The visitor can run again when the caller retries an incomplete response. */
int HttpRange_ParseResponse(const uint8_t *response, size_t length,
    uint32_t expected_start, uint32_t expected_length, uint32_t expected_total,
    size_t *body_offset, HttpRangeHeaders *metadata,
    HttpRangeHeaderFn extra, void *ctx);
#ifdef __cplusplus
}
#endif
#endif
