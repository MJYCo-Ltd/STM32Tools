#include "ML307/ml307_http.h"
#include <AT/ModuleFrameParser.h>

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
    if ((digit > maximum) || (parsed > ((maximum - digit) / 10UL)))
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

static uint8_t SuffixOk(const uint8_t *cursor, const uint8_t *end)
{
  const size_t length = (size_t)(end - cursor);
  return length == 0U ||
      (length == 1U && (*cursor == '\r' || *cursor == '\n')) ||
      (length == 2U && memcmp(cursor, "\r\n", 2U) == 0);
}

ML307_Result ML307_HttpParseUrcLine(const uint8_t *line, size_t length,
                                   ML307_HttpUrcEvent *event)
{
  static const char recv_prefix[] = "+MHTTPURC: \"recv\",";
  static const char err_prefix[] = "+MHTTPURC: \"err\",";
  const uint8_t *cursor, *end;
  uint32_t fields[4];
  const uint32_t recv_maxima[4] = {3U, 999U, UINT32_MAX, UINT32_MAX};
  const uint32_t err_maxima[2] = {3U, 255U};
  const uint32_t *maxima;
  size_t count;
  ML307_HttpUrcKind kind;
  if (event == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  memset(event, 0, sizeof(*event));
  if (line == NULL || length == 0U) return ML307_RESULT_INVALID_ARGUMENT;
  if (memchr(line, '\0', length) != NULL) return ML307_RESULT_INVALID_VALUE;
  if (length >= sizeof(recv_prefix)-1U &&
      memcmp(line, recv_prefix, sizeof(recv_prefix)-1U) == 0) {
    cursor = line + sizeof(recv_prefix)-1U;
    count = 4U; maxima = recv_maxima; kind = ML307_HTTP_URC_RECV;
  } else if (length >= sizeof(err_prefix)-1U &&
             memcmp(line, err_prefix, sizeof(err_prefix)-1U) == 0) {
    cursor = line + sizeof(err_prefix)-1U;
    count = 2U; maxima = err_maxima; kind = ML307_HTTP_URC_ERR;
  } else return ML307_RESULT_NOT_FOUND;
  end = line + length;
  for (size_t i = 0U; i < count; ++i) {
    if (ModuleFrameParser_ParseUnsigned(&cursor, end, maxima[i], &fields[i]) !=
        MODULE_FRAME_COMPLETE) return ML307_RESULT_INVALID_VALUE;
    if (i + 1U < count) {
      if (cursor >= end || *cursor != ',') return ML307_RESULT_INVALID_VALUE;
      ++cursor;
    }
  }
  if (!SuffixOk(cursor, end)) return ML307_RESULT_INVALID_VALUE;
  event->kind = kind;
  event->http_id = (uint8_t)fields[0];
  if (kind == ML307_HTTP_URC_RECV) {
    event->status_code = (uint16_t)fields[1];
    event->header_length = fields[2];
    event->content_length = fields[3];
  } else event->error_code = fields[1];
  return ML307_RESULT_OK;
}

ML307_Result ML307_HttpParseRecvUrc(const char *line, ML307_HttpRecvEvent *event)
{
  ML307_HttpUrcEvent parsed;
  ML307_Result status;
  if (line == NULL || event == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  status = ML307_HttpParseUrcLine((const uint8_t *)line, strlen(line), &parsed);
  if (status != ML307_RESULT_OK) return status;
  if (parsed.kind != ML307_HTTP_URC_RECV) return ML307_RESULT_NOT_FOUND;
  event->http_id = parsed.http_id;
  event->status_code = parsed.status_code;
  event->header_length = parsed.header_length;
  event->content_length = parsed.content_length;
  return ML307_RESULT_OK;
}

static ModuleFrameResult ParseReadTail(const uint8_t *tail, size_t available,
    uint8_t expected_id, size_t urc_capacity, size_t *consumed,
    ML307_HttpReadResult *out)
{
  const uint8_t *line;
  size_t offset = 2U, length;
  if (available < 2U) return MODULE_FRAME_INCOMPLETE;
  if (memcmp(tail, "\r\n", 2U) != 0) return MODULE_FRAME_INVALID;
  while (ModuleFrameParser_NextLine(tail, available, &offset, &line, &length)) {
    if (length == 2U && memcmp(line, "OK", 2U) == 0) {
      *consumed = offset;
      return MODULE_FRAME_COMPLETE;
    }
    if ((length == 5U && memcmp(line, "ERROR", 5U) == 0) ||
        (length >= 11U && memcmp(line, "+CME ERROR:", 11U) == 0))
      return MODULE_FRAME_INVALID;
    if (length != 0U) {
      ML307_HttpUrcEvent urc;
      if (length >= urc_capacity ||
          ML307_HttpParseUrcLine(line, length, &urc) != ML307_RESULT_OK)
        return MODULE_FRAME_INVALID;
      if (urc.http_id == expected_id && urc.kind == ML307_HTTP_URC_ERR) {
        out->module_error = urc.error_code;
        out->has_module_error = 1U;
        return MODULE_FRAME_INVALID;
      }
    }
  }
  return MODULE_FRAME_INCOMPLETE;
}

ML307_Result ML307_HttpParseReadFrame(const uint8_t *response,
    size_t response_length, uint8_t expected_http_id, uint8_t expected_data_type,
    const ML307_HttpReadLimits *limits, uint8_t *data, size_t data_capacity,
    ML307_HttpReadResult *out)
{
  const uint8_t *payload;
  uint32_t fields[4];
  size_t length, consumed, tail_consumed = 0U;
  ModuleFrameResult result;
  ModuleLengthFrameProtocol protocol = {
      "+MHTTPREAD:", 4U, 3U, (uint8_t)',', 0U, 0U, "\r\n", 2U, NULL};
  if (out == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  memset(out, 0, sizeof(*out));
  if (response == NULL || data == NULL || limits == NULL ||
      limits->urc_line_capacity == 0U || expected_http_id > ML307_HTTP_MAX_ID ||
      expected_data_type > 1U) return ML307_RESULT_INVALID_ARGUMENT;
  protocol.maximum_payload_length = limits->maximum_payload_length;
  /* First establish the exact opaque body and its CRLF boundary. Parsing the
   * rest separately permits per-call context without any global tail callback. */
  result = ModuleFrameParser_ParseLengthFrame(response, response_length,
      &protocol, fields, 4U, &payload, &length, &consumed);
  if (result != MODULE_FRAME_COMPLETE) {
    if (result == MODULE_FRAME_INVALID) return ML307_RESULT_INVALID_VALUE;
    if (ModuleFrameParser_FindLinePrefix(response, response_length, "+MHTTPREAD:") != NULL)
      return ML307_RESULT_NOT_FOUND;
    return ModuleFrameParser_HasLine(response, response_length, "ERROR", 0U) ||
           ModuleFrameParser_HasLine(response, response_length, "+CME ERROR:", 1U)
               ? ML307_RESULT_ERROR_RESPONSE : ML307_RESULT_NOT_FOUND;
  }
  result = ParseReadTail(response + consumed - 2U,
      response_length - consumed + 2U, expected_http_id,
      limits->urc_line_capacity, &tail_consumed, out);
  if (result != MODULE_FRAME_COMPLETE)
    return result == MODULE_FRAME_INVALID ? ML307_RESULT_INVALID_VALUE : ML307_RESULT_NOT_FOUND;
  if (fields[0] != expected_http_id || fields[1] != expected_data_type)
    return ML307_RESULT_INVALID_VALUE;
  if (length > data_capacity) return ML307_RESULT_BUFFER_TOO_SMALL;
  if (length != 0U) memcpy(data, payload, length);
  out->data_length = length;
  out->unread_length = fields[2];
  out->consumed = consumed - 2U + tail_consumed;
  return ML307_RESULT_OK;
}

ML307_Result ML307_HttpParseRead(const uint8_t *response, size_t response_length,
    uint8_t expected_http_id, uint8_t expected_data_type, uint8_t *data,
    size_t data_capacity, size_t *data_length, uint32_t *unread_length)
{
  const ML307_HttpReadLimits limits = {UINT32_MAX, SIZE_MAX};
  ML307_HttpReadResult out;
  ML307_Result status;
  if (data_length == NULL || unread_length == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  *data_length = 0U; *unread_length = 0U;
  status = ML307_HttpParseReadFrame(response, response_length, expected_http_id,
      expected_data_type, &limits, data, data_capacity, &out);
  if (status == ML307_RESULT_OK) {
    *data_length = out.data_length;
    *unread_length = out.unread_length;
  }
  return status;
}
