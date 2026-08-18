#include "ML307/ml307_http.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define ML307_HTTP_MAX_ID 3U

static ML307_Result FinishCommand(char *output, size_t output_size, int count,
                                  size_t *length)
{
  if ((output == NULL) || (output_size == 0U) || (length == NULL)) {
    return ML307_RESULT_INVALID_ARGUMENT;
  }
  if ((count < 0) || ((size_t)count >= output_size)) {
    output[0] = '\0';
    *length = 0U;
    return ML307_RESULT_BUFFER_TOO_SMALL;
  }
  *length = (size_t)count;
  return ML307_RESULT_OK;
}

static uint8_t IsSafeQuoted(const char *value)
{
  const unsigned char *cursor = (const unsigned char *)value;
  if ((value == NULL) || (*value == '\0')) return 0U;
  while (*cursor != '\0') {
    if ((*cursor == '"') || (*cursor == '\r') || (*cursor == '\n') ||
        (*cursor < 0x20U)) return 0U;
    ++cursor;
  }
  return 1U;
}

static ML307_Result ValidateId(uint8_t http_id)
{
  return (http_id <= ML307_HTTP_MAX_ID) ? ML307_RESULT_OK
                                         : ML307_RESULT_INVALID_VALUE;
}

ML307_Result ML307_HttpBuildCreate(char *output, size_t output_size,
                                   const char *origin, size_t *length)
{
  int count;
  if ((output == NULL) || (length == NULL) ||
      (IsSafeQuoted(origin) == 0U)) return ML307_RESULT_INVALID_ARGUMENT;
  count = snprintf(output, output_size, "AT+MHTTPCREATE=\"%s\"\r\n", origin);
  return FinishCommand(output, output_size, count, length);
}

ML307_Result ML307_HttpBuildCached(char *output, size_t output_size,
                                   uint8_t http_id, uint8_t enabled,
                                   size_t *length)
{
  int count;
  if ((output == NULL) || (length == NULL)) return ML307_RESULT_INVALID_ARGUMENT;
  if ((ValidateId(http_id) != ML307_RESULT_OK) || (enabled > 1U))
    return ML307_RESULT_INVALID_VALUE;
  count = snprintf(output, output_size, "AT+MHTTPCFG=\"cached\",%u,%u\r\n",
                   (unsigned int)http_id, (unsigned int)enabled);
  return FinishCommand(output, output_size, count, length);
}

ML307_Result ML307_HttpBuildSsl(char *output, size_t output_size,
                               uint8_t http_id, uint8_t enabled,
                               uint8_t ssl_id, size_t *length)
{
  int count;
  if ((output == NULL) || (length == NULL)) return ML307_RESULT_INVALID_ARGUMENT;
  if ((ValidateId(http_id) != ML307_RESULT_OK) || (enabled > 1U) ||
      (ssl_id > 5U))
    return ML307_RESULT_INVALID_VALUE;
  count = snprintf(output, output_size, "AT+MHTTPCFG=\"ssl\",%u,%u,%u\r\n",
                   (unsigned int)http_id, (unsigned int)enabled,
                   (unsigned int)ssl_id);
  return FinishCommand(output, output_size, count, length);
}

ML307_Result ML307_HttpBuildHeader(char *output, size_t output_size,
                                   uint8_t http_id, const char *header,
                                   size_t *length)
{
  int count;
  if ((output == NULL) || (length == NULL) ||
      (IsSafeQuoted(header) == 0U)) return ML307_RESULT_INVALID_ARGUMENT;
  if (ValidateId(http_id) != ML307_RESULT_OK) return ML307_RESULT_INVALID_VALUE;
  count = snprintf(output, output_size, "AT+MHTTPCFG=\"header\",%u,\"%s\"\r\n",
                   (unsigned int)http_id, header);
  return FinishCommand(output, output_size, count, length);
}

ML307_Result ML307_HttpBuildGet(char *output, size_t output_size,
                                uint8_t http_id, const char *path,
                                size_t *length)
{
  int count;
  if ((output == NULL) || (length == NULL) || (IsSafeQuoted(path) == 0U) ||
      (path[0] != '/')) return ML307_RESULT_INVALID_ARGUMENT;
  if (ValidateId(http_id) != ML307_RESULT_OK) return ML307_RESULT_INVALID_VALUE;
  count = snprintf(output, output_size, "AT+MHTTPREQUEST=%u,1,0,\"%s\"\r\n",
                   (unsigned int)http_id, path);
  return FinishCommand(output, output_size, count, length);
}

ML307_Result ML307_HttpBuildRead(char *output, size_t output_size,
                                 uint8_t http_id, uint8_t data_type,
                                 uint16_t read_length, size_t *length)
{
  int count;
  if ((output == NULL) || (length == NULL)) return ML307_RESULT_INVALID_ARGUMENT;
  if ((ValidateId(http_id) != ML307_RESULT_OK) || (data_type > 1U) ||
      (read_length == 0U)) return ML307_RESULT_INVALID_VALUE;
  count = snprintf(output, output_size, "AT+MHTTPREAD=%u,%u,%u\r\n",
                   (unsigned int)http_id, (unsigned int)data_type,
                   (unsigned int)read_length);
  return FinishCommand(output, output_size, count, length);
}

ML307_Result ML307_HttpBuildDelete(char *output, size_t output_size,
                                   uint8_t http_id, size_t *length)
{
  int count;
  if ((output == NULL) || (length == NULL)) return ML307_RESULT_INVALID_ARGUMENT;
  if (ValidateId(http_id) != ML307_RESULT_OK) return ML307_RESULT_INVALID_VALUE;
  count = snprintf(output, output_size, "AT+MHTTPDEL=%u\r\n",
                   (unsigned int)http_id);
  return FinishCommand(output, output_size, count, length);
}

static ML307_Result ParseUnsigned(const char **cursor, unsigned long maximum,
                                  unsigned long *value)
{
  unsigned long parsed = 0UL;
  uint8_t have_digit = 0U;
  while ((**cursor >= '0') && (**cursor <= '9')) {
    const unsigned long digit = (unsigned long)(**cursor - '0');
    if ((digit > maximum) ||
        (parsed > ((maximum - digit) / 10UL)))
      return ML307_RESULT_INVALID_VALUE;
    parsed = (parsed * 10UL) + digit;
    have_digit = 1U;
    ++(*cursor);
  }
  if (have_digit == 0U) return ML307_RESULT_NOT_FOUND;
  *value = parsed;
  return ML307_RESULT_OK;
}

ML307_Result ML307_HttpParseCreate(const char *response, uint8_t *http_id)
{
  const char *cursor;
  unsigned long parsed;
  if ((response == NULL) || (http_id == NULL)) return ML307_RESULT_INVALID_ARGUMENT;
  cursor = strstr(response, "+MHTTPCREATE:");
  if (cursor == NULL)
    return (strstr(response, "ERROR") != NULL) ? ML307_RESULT_ERROR_RESPONSE
                                                : ML307_RESULT_NOT_FOUND;
  cursor += strlen("+MHTTPCREATE:");
  while (*cursor == ' ') ++cursor;
  if ((ParseUnsigned(&cursor, ML307_HTTP_MAX_ID, &parsed) != ML307_RESULT_OK) ||
      ((*cursor != '\r') && (*cursor != '\n') && (*cursor != '\0')))
    return ML307_RESULT_INVALID_VALUE;
  *http_id = (uint8_t)parsed;
  return ML307_RESULT_OK;
}

ML307_Result ML307_HttpParseRecvUrc(const char *line,
                                    ML307_HttpRecvEvent *event)
{
  const char *cursor;
  unsigned long fields[4];
  size_t i;
  if ((line == NULL) || (event == NULL)) return ML307_RESULT_INVALID_ARGUMENT;
  cursor = strstr(line, "+MHTTPURC:");
  if (cursor == NULL) return ML307_RESULT_NOT_FOUND;
  cursor += strlen("+MHTTPURC:");
  while (*cursor == ' ') ++cursor;
  if (strncmp(cursor, "\"recv\",", 7U) != 0) return ML307_RESULT_NOT_FOUND;
  cursor += 7U;
  for (i = 0U; i < 4U; ++i) {
    const unsigned long maximum = (i == 0U) ? ML307_HTTP_MAX_ID
                                  : ((i == 1U) ? 999UL : ULONG_MAX);
    ML307_Result result = ParseUnsigned(&cursor, maximum, &fields[i]);
    if (result != ML307_RESULT_OK) return result;
    if (i < 3U) {
      if (*cursor != ',') return ML307_RESULT_INVALID_VALUE;
      ++cursor;
    }
  }
  if ((*cursor != '\r') && (*cursor != '\n') && (*cursor != '\0'))
    return ML307_RESULT_INVALID_VALUE;
  event->http_id = (uint8_t)fields[0];
  event->status_code = (uint16_t)fields[1];
  event->header_length = (uint32_t)fields[2];
  event->content_length = (uint32_t)fields[3];
  return ML307_RESULT_OK;
}

static const uint8_t *FindBytes(const uint8_t *data, size_t length,
                                const char *needle)
{
  const size_t needle_length = strlen(needle);
  size_t i;
  if ((data == NULL) || (needle_length == 0U) || (length < needle_length))
    return NULL;
  for (i = 0U; i <= (length - needle_length); ++i)
    if (memcmp(data + i, needle, needle_length) == 0) return data + i;
  return NULL;
}

static ML307_Result ParseBoundedUnsigned(const uint8_t **cursor,
                                         const uint8_t *end,
                                         unsigned long maximum,
                                         unsigned long *value)
{
  unsigned long parsed = 0UL;
  uint8_t have_digit = 0U;
  while ((*cursor < end) && (**cursor >= (uint8_t)'0') &&
         (**cursor <= (uint8_t)'9')) {
    const unsigned long digit = (unsigned long)(**cursor - (uint8_t)'0');
    if ((digit > maximum) ||
        (parsed > ((maximum - digit) / 10UL)))
      return ML307_RESULT_INVALID_VALUE;
    parsed = (parsed * 10UL) + digit;
    have_digit = 1U;
    ++(*cursor);
  }
  if (have_digit == 0U) return ML307_RESULT_NOT_FOUND;
  *value = parsed;
  return ML307_RESULT_OK;
}

ML307_Result ML307_HttpParseRead(const uint8_t *response,
                                 size_t response_length,
                                 uint8_t expected_http_id,
                                 uint8_t expected_data_type, uint8_t *data,
                                 size_t data_capacity, size_t *data_length,
                                 uint32_t *unread_length)
{
  static const char prefix[] = "+MHTTPREAD:";
  const uint8_t *cursor;
  const uint8_t *end;
  const uint8_t *payload;
  const uint8_t *tail;
  unsigned long fields[4];
  size_t i;
  if ((response == NULL) || (data == NULL) || (data_length == NULL) ||
      (unread_length == NULL)) return ML307_RESULT_INVALID_ARGUMENT;
  *data_length = 0U;
  *unread_length = 0U;
  cursor = FindBytes(response, response_length, prefix);
  if (cursor == NULL)
    return (FindBytes(response, response_length, "ERROR") != NULL)
               ? ML307_RESULT_ERROR_RESPONSE : ML307_RESULT_NOT_FOUND;
  cursor += sizeof(prefix) - 1U;
  end = response + response_length;
  while ((cursor < end) && (*cursor == (uint8_t)' ')) ++cursor;
  for (i = 0U; i < 4U; ++i) {
    const unsigned long maximum = (i == 0U) ? ML307_HTTP_MAX_ID
                                  : ((i == 1U) ? 1UL : ULONG_MAX);
    ML307_Result result = ParseBoundedUnsigned(&cursor, end, maximum, &fields[i]);
    if (result != ML307_RESULT_OK) return result;
    if ((cursor >= end) || (*cursor != (uint8_t)','))
      return ML307_RESULT_INVALID_VALUE;
    ++cursor;
  }
  payload = cursor;
  if ((fields[0] != expected_http_id) || (fields[1] != expected_data_type))
    return ML307_RESULT_INVALID_VALUE;
  if (fields[3] > data_capacity) return ML307_RESULT_BUFFER_TOO_SMALL;
  if ((size_t)(end - payload) < (size_t)fields[3]) return ML307_RESULT_NOT_FOUND;
  tail = payload + (size_t)fields[3];
  if (((size_t)(end - tail) < 6U) ||
      (memcmp(tail, "\r\nOK\r\n", 6U) != 0))
    return ML307_RESULT_NOT_FOUND;
  if (fields[3] > 0UL) memcpy(data, payload, (size_t)fields[3]);
  *data_length = (size_t)fields[3];
  *unread_length = (uint32_t)fields[2];
  return ML307_RESULT_OK;
}
