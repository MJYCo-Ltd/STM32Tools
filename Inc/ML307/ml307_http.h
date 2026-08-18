/* ML307 HTTP cached/range command helpers. */
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
ML307_Result ML307_HttpParseRecvUrc(const char *line,
                                    ML307_HttpRecvEvent *event);

/* Parse by declared byte length; binary data may contain NUL/CR/LF/comma. */
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
