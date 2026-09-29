#include <ML307/ml307_ssl.h>
#include <ML307/ml307_mqtt.h>

#include <AT/ModuleFrameParser.h>
#include <AT/at_codec.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  const char *wire_name;
  uint32_t secure_value;
} SslOptionInfo;

static const SslOptionInfo s_options[] = {
    {"auth", 1U}, {"version", 3U}, {"ignorestamp", 0U},
    {"ignoreverify", 0U}, {"encoding", 2U}, {"cert", 0U}};

static ML307_Result SslFormat(char *output, size_t output_size,
                             const char *format, ...)
{
  AT_CodecResult result;
  va_list args;
  va_start(args, format);
  result = AT_FormatV(output, output_size, format, args);
  va_end(args);
  if (result == AT_CODEC_OK) return ML307_RESULT_OK;
  if (result == AT_CODEC_BUFFER_TOO_SMALL)
    return ML307_RESULT_BUFFER_TOO_SMALL;
  return ML307_RESULT_INVALID_ARGUMENT;
}

static const SslOptionInfo *SslGetOption(ML307_SslOption option)
{
  if ((unsigned int)option >=
      (unsigned int)(sizeof(s_options) / sizeof(s_options[0]))) return NULL;
  return &s_options[(unsigned int)option];
}

static int SslNameIsValid(const char *name, int allow_empty)
{
  size_t length;
  if (name == NULL) return 0;
  length = strlen(name);
  if ((!allow_empty && length == 0U) || length > ML307_SSL_CERT_NAME_MAX)
    return 0;
  return strchr(name, '"') == NULL && strchr(name, ',') == NULL &&
         strchr(name, '\r') == NULL && strchr(name, '\n') == NULL;
}

ML307_Result ML307_SslBuildSetOption(char *output, size_t output_size,
                                    ML307_SslOption option, uint8_t ssl_id,
                                    uint32_t value)
{
  const SslOptionInfo *info = SslGetOption(option);
  if (info == NULL || option == ML307_SSL_OPTION_CERTIFICATE ||
      ssl_id > ML307_SSL_CONTEXT_MAX || value != info->secure_value)
    return ML307_RESULT_INVALID_VALUE;
  return SslFormat(output, output_size, "AT+MSSLCFG=\"%s\",%u,%lu\r\n",
                   info->wire_name, (unsigned int)ssl_id,
                   (unsigned long)value);
}

ML307_Result ML307_SslBuildQueryOption(char *output, size_t output_size,
                                      ML307_SslOption option, uint8_t ssl_id)
{
  const SslOptionInfo *info = SslGetOption(option);
  if (info == NULL || ssl_id > ML307_SSL_CONTEXT_MAX)
    return ML307_RESULT_INVALID_VALUE;
  return SslFormat(output, output_size, "AT+MSSLCFG=\"%s\",%u\r\n",
                   info->wire_name, (unsigned int)ssl_id);
}

static int SslIsErrorLine(const AT_Line *line)
{
  return AT_LineEquals(line, "ERROR") ||
         AT_LineStartsWith(line, "+CME ERROR:") ||
         AT_LineStartsWith(line, "+CMS ERROR:") ||
         AT_LineStartsWith(line, "+CIS ERROR:");
}

typedef struct {
  unsigned int ok_count;
  unsigned int error_count;
  unsigned int meaningful_after_final;
  unsigned int final_seen;
} SslFinalScan;

static ML307_SslParseResult SslScanFinal(const uint8_t *response,
                                        size_t length, SslFinalScan *scan)
{
  size_t offset = 0U;
  const uint8_t *line;
  size_t line_length;
  if (response == NULL || scan == NULL) return ML307_SSL_PARSE_INVALID;
  memset(scan, 0, sizeof(*scan));
  while (ModuleFrameParser_NextLine(response, length, &offset, &line,
                                    &line_length) != 0U) {
    AT_Line bounded_line = {(const char *)line, line_length};
    AT_TrimLine(&bounded_line);
    if (bounded_line.length == 0U) continue;
    if (scan->final_seen != 0U) ++scan->meaningful_after_final;
    if (AT_LineEquals(&bounded_line, "OK")) {
      ++scan->ok_count;
      scan->final_seen = 1U;
    } else if (SslIsErrorLine(&bounded_line)) {
      ++scan->error_count;
      scan->final_seen = 1U;
    }
  }
  if (offset != length) return ML307_SSL_PARSE_INCOMPLETE;
  if (scan->meaningful_after_final != 0U ||
      (scan->ok_count + scan->error_count) > 1U)
    return ML307_SSL_PARSE_INVALID;
  if (scan->error_count == 1U) return ML307_SSL_PARSE_ERROR_RESPONSE;
  if (scan->ok_count == 0U) return ML307_SSL_PARSE_INCOMPLETE;
  return ML307_SSL_PARSE_COMPLETE;
}

static int SslValueIsValid(ML307_SslOption option, uint32_t value)
{
  switch (option) {
  case ML307_SSL_OPTION_AUTH:
    return value <= (uint32_t)ML307_SSL_AUTH_MUTUAL;
  case ML307_SSL_OPTION_VERSION:
    return value <= (uint32_t)ML307_SSL_VERSION_TLS12 ||
           value == (uint32_t)ML307_SSL_VERSION_ALL;
  case ML307_SSL_OPTION_IGNORE_TIMESTAMP:
  case ML307_SSL_OPTION_IGNORE_VERIFICATION:
    return value <= 1U;
  case ML307_SSL_OPTION_ENCODING:
    return value == (uint32_t)ML307_SSL_ENCODING_ESCAPED;
  default:
    return 0;
  }
}

static ML307_SslParseResult SslParseNumericLine(
    const uint8_t *line, size_t length, ML307_SslOption option,
    uint8_t expected_id, uint32_t *parsed_value)
{
  static const char prefix[] = "+MSSLCFG: \"";
  const SslOptionInfo *info = SslGetOption(option);
  const uint8_t *cursor;
  const uint8_t *end;
  uint32_t id;
  uint32_t value;
  size_t key_length;
  if (info == NULL || parsed_value == NULL ||
      length < sizeof(prefix) - 1U ||
      memcmp(line, prefix, sizeof(prefix) - 1U) != 0)
    return ML307_SSL_PARSE_INVALID;
  cursor = line + sizeof(prefix) - 1U;
  end = line + length;
  key_length = strlen(info->wire_name);
  if ((size_t)(end - cursor) < key_length + 2U ||
      memcmp(cursor, info->wire_name, key_length) != 0 ||
      cursor[key_length] != '"' || cursor[key_length + 1U] != ',')
    return ML307_SSL_PARSE_INVALID;
  cursor += key_length + 2U;
  if (ModuleFrameParser_ParseUnsigned(&cursor, end, ML307_SSL_CONTEXT_MAX,
                                      &id) != MODULE_FRAME_COMPLETE ||
      cursor == end || *cursor++ != ',' ||
      ModuleFrameParser_ParseUnsigned(&cursor, end, UINT32_MAX, &value) !=
          MODULE_FRAME_COMPLETE ||
      cursor != end || id != expected_id || !SslValueIsValid(option, value))
    return ML307_SSL_PARSE_INVALID;
  *parsed_value = value;
  return ML307_SSL_PARSE_COMPLETE;
}

ML307_SslParseResult ML307_SslParseOptionQuery(
    const uint8_t *response, size_t length, ML307_SslOption option,
    uint8_t ssl_id, ML307_SslOptionValue *value)
{
  size_t offset = 0U;
  const uint8_t *line;
  size_t line_length;
  uint32_t parsed = 0U;
  unsigned int matches = 0U;
  SslFinalScan final_scan;
  ML307_SslParseResult final_result;
  if (response == NULL || value == NULL || SslGetOption(option) == NULL ||
      option == ML307_SSL_OPTION_CERTIFICATE ||
      ssl_id > ML307_SSL_CONTEXT_MAX) return ML307_SSL_PARSE_INVALID;
  final_result = SslScanFinal(response, length, &final_scan);
  if (final_result != ML307_SSL_PARSE_COMPLETE) return final_result;
  while (ModuleFrameParser_NextLine(response, length, &offset, &line,
                                    &line_length) != 0U) {
    AT_Line bounded_line = {(const char *)line, line_length};
    uint32_t candidate;
    AT_TrimLine(&bounded_line);
    if (!AT_LineStartsWith(&bounded_line, "+MSSLCFG:")) continue;
    if (SslParseNumericLine((const uint8_t *)bounded_line.data,
                            bounded_line.length, option, ssl_id, &candidate) !=
        ML307_SSL_PARSE_COMPLETE) return ML307_SSL_PARSE_INVALID;
    parsed = candidate;
    ++matches;
  }
  if (matches != 1U) return ML307_SSL_PARSE_INVALID;
  value->option = option;
  value->ssl_id = ssl_id;
  value->value = parsed;
  return ML307_SSL_PARSE_COMPLETE;
}

ML307_Result ML307_SslBuildSetCa(char *output, size_t output_size,
                                uint8_t ssl_id, const char *ca_name)
{
  if (ssl_id > ML307_SSL_CONTEXT_MAX || !SslNameIsValid(ca_name, 0))
    return ML307_RESULT_INVALID_VALUE;
  return SslFormat(output, output_size,
                   "AT+MSSLCFG=\"cert\",%u,\"%s\",\"\",\"\"\r\n",
                   (unsigned int)ssl_id, ca_name);
}

static int SslParseQuotedName(const uint8_t **cursor, const uint8_t *end,
                              char *output)
{
  const uint8_t *begin;
  size_t length;
  if (*cursor >= end || **cursor != '"') return 0;
  ++(*cursor);
  begin = *cursor;
  while (*cursor < end && **cursor != '"') ++(*cursor);
  if (*cursor >= end) return 0;
  length = (size_t)(*cursor - begin);
  if (length > ML307_SSL_CERT_NAME_MAX ||
      memchr(begin, '\0', length) != NULL || memchr(begin, ',', length) != NULL ||
      memchr(begin, '\r', length) != NULL ||
      memchr(begin, '\n', length) != NULL) return 0;
  if (length > 0U) memcpy(output, begin, length);
  output[length] = '\0';
  ++(*cursor);
  /* ML307C reports unset certificate slots as the literal token NULL. */
  if (strcmp(output, "NULL") == 0) output[0] = '\0';
  return 1;
}

static ML307_SslParseResult SslParseCertificateLine(
    const uint8_t *line, size_t length, uint8_t expected_id,
    ML307_SslCertificateNames *names)
{
  static const char prefix[] = "+MSSLCFG: \"cert\",";
  const uint8_t *cursor;
  const uint8_t *end;
  uint32_t id;
  ML307_SslCertificateNames candidate;
  if (length < sizeof(prefix) - 1U ||
      memcmp(line, prefix, sizeof(prefix) - 1U) != 0)
    return ML307_SSL_PARSE_INVALID;
  memset(&candidate, 0, sizeof(candidate));
  cursor = line + sizeof(prefix) - 1U;
  end = line + length;
  if (ModuleFrameParser_ParseUnsigned(&cursor, end, ML307_SSL_CONTEXT_MAX,
                                      &id) != MODULE_FRAME_COMPLETE ||
      id != expected_id || cursor == end || *cursor++ != ',' ||
      !SslParseQuotedName(&cursor, end, candidate.server_ca) ||
      cursor == end || *cursor++ != ',' ||
      !SslParseQuotedName(&cursor, end, candidate.client_certificate) ||
      cursor == end || *cursor++ != ',' ||
      !SslParseQuotedName(&cursor, end, candidate.private_key) || cursor != end)
    return ML307_SSL_PARSE_INVALID;
  *names = candidate;
  return ML307_SSL_PARSE_COMPLETE;
}

ML307_SslParseResult ML307_SslParseCertificateQuery(
    const uint8_t *response, size_t length, uint8_t ssl_id,
    ML307_SslCertificateNames *names)
{
  size_t offset = 0U;
  const uint8_t *line;
  size_t line_length;
  unsigned int matches = 0U;
  ML307_SslCertificateNames candidate;
  SslFinalScan scan;
  ML307_SslParseResult result;
  if (response == NULL || names == NULL || ssl_id > ML307_SSL_CONTEXT_MAX)
    return ML307_SSL_PARSE_INVALID;
  result = SslScanFinal(response, length, &scan);
  if (result != ML307_SSL_PARSE_COMPLETE) return result;
  while (ModuleFrameParser_NextLine(response, length, &offset, &line,
                                    &line_length) != 0U) {
    AT_Line bounded_line = {(const char *)line, line_length};
    ML307_SslCertificateNames parsed;
    AT_TrimLine(&bounded_line);
    if (!AT_LineStartsWith(&bounded_line, "+MSSLCFG:")) continue;
    if (SslParseCertificateLine((const uint8_t *)bounded_line.data,
                                bounded_line.length, ssl_id, &parsed) !=
        ML307_SSL_PARSE_COMPLETE) return ML307_SSL_PARSE_INVALID;
    candidate = parsed;
    ++matches;
  }
  if (matches != 1U) return ML307_SSL_PARSE_INVALID;
  *names = candidate;
  return ML307_SSL_PARSE_COMPLETE;
}

static int SslCertificateChunkIsValid(const ML307_SslCertificateChunk *chunk)
{
  return chunk != NULL && SslNameIsValid(chunk->name, 0) &&
         chunk->certificate != NULL && chunk->certificate_length > 0U &&
         chunk->certificate_length <= ML307_SSL_CERTIFICATE_MAX &&
         chunk->offset < chunk->certificate_length && chunk->length > 0U &&
         chunk->length <= chunk->certificate_length - chunk->offset;
}

ML307_Result ML307_SslBuildCertificateWrite(
    char *output, size_t output_size,
    const ML307_SslCertificateChunk *chunk)
{
  size_t remaining;
  if (!SslCertificateChunkIsValid(chunk)) return ML307_RESULT_INVALID_VALUE;
  remaining = chunk->certificate_length - chunk->offset - chunk->length;
  return SslFormat(output, output_size, "AT+MSSLCERTWR=\"%s\",%lu,%lu\r\n",
                   chunk->name, (unsigned long)remaining,
                   (unsigned long)chunk->length);
}

ML307_Result ML307_SslGetCertificateWritePayload(
    const ML307_SslCertificateChunk *chunk, const uint8_t **payload,
    size_t *payload_length)
{
  if (!SslCertificateChunkIsValid(chunk) || payload == NULL ||
      payload_length == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  *payload = chunk->certificate + chunk->offset;
  *payload_length = chunk->length;
  return ML307_RESULT_OK;
}

ML307_SslParseResult ML307_SslParseCertificateWritePrompt(
    const uint8_t *response, size_t length)
{
  size_t i;
  if (response == NULL) return ML307_SSL_PARSE_INVALID;
  for (i = 0U; i < length; ++i) {
    if (response[i] == '>' && (i == 0U || response[i - 1U] == '\n')) {
      size_t prefix_offset = 0U;
      const uint8_t *prefix_line;
      size_t prefix_line_length;
      size_t tail;
      for (tail = i + 1U; tail < length; ++tail) {
        if (response[tail] != ' ' && response[tail] != '\t' &&
            response[tail] != '\r' && response[tail] != '\n')
          return ML307_SSL_PARSE_INVALID;
      }
      while (ModuleFrameParser_NextLine(response, i, &prefix_offset,
                                        &prefix_line,
                                        &prefix_line_length) != 0U) {
        AT_Line bounded_line = {(const char *)prefix_line,
                                prefix_line_length};
        AT_TrimLine(&bounded_line);
        if (AT_LineEquals(&bounded_line, "OK") ||
            SslIsErrorLine(&bounded_line))
          return ML307_SSL_PARSE_INVALID;
      }
      if (prefix_offset != i) return ML307_SSL_PARSE_INVALID;
      return ML307_SSL_PARSE_COMPLETE;
    }
  }
  {
    size_t offset = 0U;
    const uint8_t *line;
    size_t line_length;
    while (ModuleFrameParser_NextLine(response, length, &offset, &line,
                                      &line_length) != 0U) {
      AT_Line bounded_line = {(const char *)line, line_length};
      AT_TrimLine(&bounded_line);
      if (SslIsErrorLine(&bounded_line))
        return ML307_SSL_PARSE_ERROR_RESPONSE;
      if (AT_LineEquals(&bounded_line, "OK"))
        return ML307_SSL_PARSE_INVALID;
    }
  }
  return ML307_SSL_PARSE_INCOMPLETE;
}

ML307_SslParseResult ML307_SslParseFinalResult(const uint8_t *response,
                                               size_t length)
{
  SslFinalScan scan;
  return SslScanFinal(response, length, &scan);
}

ML307_Result ML307_SslBuildCertificateRead(char *output, size_t output_size,
                                           const char *name)
{
  if (!SslNameIsValid(name, 0)) return ML307_RESULT_INVALID_VALUE;
  return SslFormat(output, output_size, "AT+MSSLCERTRD=\"%s\"\r\n", name);
}

static ModuleFrameResult SslCertificatePayloadBoundary(
    const uint8_t *tail, size_t available, size_t *consumed)
{
  (void)tail;
  (void)available;
  if (consumed == NULL) return MODULE_FRAME_INVALID;
  *consumed = 0U;
  return MODULE_FRAME_COMPLETE;
}

/* ML307C returns either "\r\nOK\r\n" or an extra blank CRLF before OK after
 * +MSSLCERTRD payloads. Accept a bounded number of CRLF separators, then OK. */
static ModuleFrameResult SslCertificateReadTail(const uint8_t *tail,
                                                size_t available,
                                                size_t *consumed)
{
  size_t offset = 0U;
  if (consumed == NULL || tail == NULL) return MODULE_FRAME_INVALID;
  *consumed = 0U;
  if (available < 6U) return MODULE_FRAME_INCOMPLETE;
  while (offset + 6U <= available &&
         tail[offset] == (uint8_t)'\r' &&
         tail[offset + 1U] == (uint8_t)'\n') {
    if (memcmp(tail + offset, "\r\nOK\r\n", 6U) == 0) {
      *consumed = offset + 6U;
      return MODULE_FRAME_COMPLETE;
    }
    offset += 2U;
    if (offset > 8U) return MODULE_FRAME_INVALID;
  }
  if (offset < available) return MODULE_FRAME_INVALID;
  return MODULE_FRAME_INCOMPLETE;
}

static uint8_t SslControlHasModuleReady(const uint8_t *data, size_t length)
{
  return ModuleFrameParser_HasLine(data, length, "+MATREADY", 0U);
}

static uint8_t SslControlHasModuleReadyBeforeCertificateFrame(
    const uint8_t *data, size_t length)
{
  const uint8_t *certificate_frame =
      ModuleFrameParser_FindLinePrefix(data, length, "+MSSLCERTRD:");
  const size_t control_length =
      (certificate_frame == NULL) ? length
                                  : (size_t)(certificate_frame - data);
  return SslControlHasModuleReady(data, control_length);
}

ML307_SslParseResult ML307_SslParseCertificateRead(
    const uint8_t *response, size_t length, uint8_t *certificate,
    size_t certificate_capacity, size_t *certificate_length)
{
  static const ModuleLengthFrameProtocol protocol = {
      "+MSSLCERTRD:", 1U, 0U, ',', 0U, ML307_SSL_CERTIFICATE_MAX,
      NULL, 0U, SslCertificateReadTail};
  static const ModuleLengthFrameProtocol boundary_protocol = {
      "+MSSLCERTRD:", 1U, 0U, ',', 0U, ML307_SSL_CERTIFICATE_MAX,
      NULL, 0U, SslCertificatePayloadBoundary};
  uint32_t fields[1];
  uint32_t boundary_fields[1];
  const uint8_t *frame_start;
  const uint8_t *payload;
  const uint8_t *boundary_payload;
  size_t payload_length;
  size_t boundary_payload_length;
  size_t consumed;
  size_t boundary_consumed;
  ModuleFrameResult result;
  ModuleFrameResult boundary_result;
  if (response == NULL || certificate == NULL || certificate_length == NULL)
    return ML307_SSL_PARSE_INVALID;
  frame_start = ModuleFrameParser_FindLinePrefix(response, length,
                                                  protocol.prefix);
  if (frame_start == NULL) {
    SslFinalScan scan;
    ML307_SslParseResult final_result = SslScanFinal(response, length, &scan);
    if (SslControlHasModuleReady(response, length) != 0U)
      return ML307_SSL_PARSE_MODULE_RESET;
    return (final_result == ML307_SSL_PARSE_ERROR_RESPONSE)
               ? final_result
               : ((final_result == ML307_SSL_PARSE_INCOMPLETE)
                      ? ML307_SSL_PARSE_INCOMPLETE
                      : ML307_SSL_PARSE_INVALID);
  }
  {
    const size_t prefix_length = (size_t)(frame_start - response);
    size_t offset = 0U;
    const uint8_t *line;
    size_t line_length;
    if (SslControlHasModuleReady(response, prefix_length) != 0U)
      return ML307_SSL_PARSE_MODULE_RESET;
    while (ModuleFrameParser_NextLine(response, prefix_length, &offset, &line,
                                      &line_length) != 0U) {
      AT_Line bounded_line = {(const char *)line, line_length};
      AT_TrimLine(&bounded_line);
      if (AT_LineEquals(&bounded_line, "OK") ||
          SslIsErrorLine(&bounded_line))
        return ML307_SSL_PARSE_INVALID;
    }
    if (offset != prefix_length) return ML307_SSL_PARSE_INVALID;
  }
  result = ModuleFrameParser_ParseLengthFrame(
      response, length, &protocol, fields, 1U, &payload, &payload_length,
      &consumed);
  boundary_result = ModuleFrameParser_ParseLengthFrame(
      response, length, &boundary_protocol, boundary_fields, 1U,
      &boundary_payload, &boundary_payload_length, &boundary_consumed);
  if (boundary_result == MODULE_FRAME_COMPLETE && boundary_consumed < length &&
      SslControlHasModuleReadyBeforeCertificateFrame(
          response + boundary_consumed, length - boundary_consumed) != 0U)
    return ML307_SSL_PARSE_MODULE_RESET;
  if (result == MODULE_FRAME_INCOMPLETE) return ML307_SSL_PARSE_INCOMPLETE;
  if (result != MODULE_FRAME_COMPLETE || consumed != length ||
      payload_length == 0U) return ML307_SSL_PARSE_INVALID;
  if (payload_length > certificate_capacity)
    return ML307_SSL_PARSE_BUFFER_TOO_SMALL;
  memcpy(certificate, payload, payload_length);
  *certificate_length = payload_length;
  return ML307_SSL_PARSE_COMPLETE;
}

static int SslClockIsValid(const ML307_ModuleClock *clock)
{
  static const uint8_t days[] = {31U, 28U, 31U, 30U, 31U, 30U,
                                 31U, 31U, 30U, 31U, 30U, 31U};
  uint8_t maximum_day;
  if (clock == NULL || clock->year < 1970U || clock->year > 2037U ||
      clock->month < 1U || clock->month > 12U || clock->hour > 23U ||
      clock->minute > 59U || clock->second > 59U ||
      clock->timezone_quarters < -48 || clock->timezone_quarters > 56 ||
      clock->timezone_quarters == -1 || clock->timezone_quarters == 1)
    return 0;
  maximum_day = days[clock->month - 1U];
  if (clock->month == 2U && (clock->year % 4U) == 0U) maximum_day = 29U;
  return clock->day >= 1U && clock->day <= maximum_day;
}

ML307_Result ML307_SslBuildClockSet(char *output, size_t output_size,
                                   const ML307_ModuleClock *clock)
{
  unsigned int zone;
  char sign;
  if (!SslClockIsValid(clock)) return ML307_RESULT_INVALID_VALUE;
  sign = clock->timezone_quarters < 0 ? '-' : '+';
  zone = (unsigned int)(clock->timezone_quarters < 0
                            ? -clock->timezone_quarters
                            : clock->timezone_quarters);
  return SslFormat(output, output_size,
                   "AT+CCLK=\"%02u/%02u/%02u,%02u:%02u:%02u%c%02u\"\r\n",
                   (unsigned int)(clock->year % 100U),
                   (unsigned int)clock->month, (unsigned int)clock->day,
                   (unsigned int)clock->hour, (unsigned int)clock->minute,
                   (unsigned int)clock->second, sign, zone);
}

ML307_Result ML307_SslBuildClockQuery(char *output, size_t output_size)
{
  return SslFormat(output, output_size, "AT+CCLK?\r\n");
}

static int SslTwoDigits(const uint8_t **cursor, const uint8_t *end,
                        uint32_t *value)
{
  if ((size_t)(end - *cursor) < 2U || (*cursor)[0] < '0' ||
      (*cursor)[0] > '9' || (*cursor)[1] < '0' || (*cursor)[1] > '9')
    return 0;
  *value = (uint32_t)((*cursor)[0] - '0') * 10U +
           (uint32_t)((*cursor)[1] - '0');
  *cursor += 2U;
  return 1;
}

static int SslConsume(const uint8_t **cursor, const uint8_t *end, uint8_t byte)
{
  if (*cursor >= end || **cursor != byte) return 0;
  ++(*cursor);
  return 1;
}

static int SslParseClockLine(const uint8_t *line, size_t length,
                             ML307_ModuleClock *clock)
{
  static const char prefix[] = "+CCLK: \"";
  const uint8_t *cursor;
  const uint8_t *end;
  uint32_t year, month, day, hour, minute, second, zone;
  int sign;
  ML307_ModuleClock candidate;
  if (length < sizeof(prefix) - 1U ||
      memcmp(line, prefix, sizeof(prefix) - 1U) != 0) return 0;
  cursor = line + sizeof(prefix) - 1U;
  end = line + length;
  if (!SslTwoDigits(&cursor, end, &year) ||
      !SslConsume(&cursor, end, '/') ||
      !SslTwoDigits(&cursor, end, &month) ||
      !SslConsume(&cursor, end, '/') || !SslTwoDigits(&cursor, end, &day) ||
      !SslConsume(&cursor, end, ',') || !SslTwoDigits(&cursor, end, &hour) ||
      !SslConsume(&cursor, end, ':') ||
      !SslTwoDigits(&cursor, end, &minute) ||
      !SslConsume(&cursor, end, ':') ||
      !SslTwoDigits(&cursor, end, &second) || cursor >= end ||
      (*cursor != '+' && *cursor != '-')) return 0;
  sign = *cursor++ == '-' ? -1 : 1;
  if (!SslTwoDigits(&cursor, end, &zone) || !SslConsume(&cursor, end, '"') ||
      cursor != end || zone > 99U) return 0;
  candidate.year = (uint16_t)((year >= 70U) ? (1900U + year)
                                            : (2000U + year));
  candidate.month = (uint8_t)month;
  candidate.day = (uint8_t)day;
  candidate.hour = (uint8_t)hour;
  candidate.minute = (uint8_t)minute;
  candidate.second = (uint8_t)second;
  candidate.timezone_quarters = (int8_t)((int)zone * sign);
  if (!SslClockIsValid(&candidate)) return 0;
  *clock = candidate;
  return 1;
}

ML307_SslParseResult ML307_SslParseClockQuery(const uint8_t *response,
                                              size_t length,
                                              ML307_ModuleClock *clock)
{
  size_t offset = 0U;
  const uint8_t *line;
  size_t line_length;
  unsigned int matches = 0U;
  ML307_ModuleClock candidate;
  SslFinalScan scan;
  ML307_SslParseResult result;
  if (response == NULL || clock == NULL) return ML307_SSL_PARSE_INVALID;
  result = SslScanFinal(response, length, &scan);
  if (result != ML307_SSL_PARSE_COMPLETE) return result;
  while (ModuleFrameParser_NextLine(response, length, &offset, &line,
                                    &line_length) != 0U) {
    AT_Line bounded_line = {(const char *)line, line_length};
    ML307_ModuleClock parsed;
    AT_TrimLine(&bounded_line);
    if (!AT_LineStartsWith(&bounded_line, "+CCLK:")) continue;
    if (!SslParseClockLine((const uint8_t *)bounded_line.data,
                           bounded_line.length, &parsed))
      return ML307_SSL_PARSE_INVALID;
    candidate = parsed;
    ++matches;
  }
  if (matches != 1U) return ML307_SSL_PARSE_INVALID;
  *clock = candidate;
  return ML307_SSL_PARSE_COMPLETE;
}

enum {
  SSL_TX_SEND_CERT_WRITE = 1,
  SSL_TX_WAIT_CERT_PROMPT,
  SSL_TX_SEND_CERT_PAYLOAD,
  SSL_TX_WAIT_CERT_FINAL,
  SSL_TX_SEND_CERT_READ,
  SSL_TX_WAIT_CERT_READ,
  SSL_TX_SEND_OPTION_SET = 20,
  SSL_TX_WAIT_OPTION_SET,
  SSL_TX_SEND_OPTION_QUERY,
  SSL_TX_WAIT_OPTION_QUERY,
  SSL_TX_SEND_CA_SET,
  SSL_TX_WAIT_CA_SET,
  SSL_TX_SEND_CA_QUERY,
  SSL_TX_WAIT_CA_QUERY,
  SSL_TX_SEND_CLOCK_SET,
  SSL_TX_WAIT_CLOCK_SET,
  SSL_TX_SEND_CLOCK_QUERY,
  SSL_TX_WAIT_CLOCK_QUERY,
  SSL_TX_SEND_MQTT_SET,
  SSL_TX_WAIT_MQTT_SET,
  SSL_TX_SEND_MQTT_QUERY,
  SSL_TX_WAIT_MQTT_QUERY
};

static const ML307_SslOption s_prepare_options[] = {
    ML307_SSL_OPTION_ENCODING, ML307_SSL_OPTION_AUTH,
    ML307_SSL_OPTION_VERSION, ML307_SSL_OPTION_IGNORE_TIMESTAMP,
    ML307_SSL_OPTION_IGNORE_VERIFICATION};

static uint32_t SslPrepareValue(ML307_SslOption option)
{
  const SslOptionInfo *info = SslGetOption(option);
  return info == NULL ? UINT32_MAX : info->secure_value;
}

ML307_Result ML307_SslClockFromUnixUtc(uint32_t unix_time,
                                       ML307_ModuleClock *clock)
{
  static const uint8_t days[] = {31U, 28U, 31U, 30U, 31U, 30U,
                                 31U, 31U, 30U, 31U, 30U, 31U};
  uint32_t day_count;
  uint32_t seconds;
  uint16_t year = 1970U;
  uint8_t month = 1U;
  uint32_t span;
  if (clock == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  day_count = unix_time / 86400U;
  seconds = unix_time % 86400U;
  while (year <= 2037U) {
    span = ((year % 4U) == 0U) ? 366U : 365U;
    if (day_count < span) break;
    day_count -= span;
    ++year;
  }
  if (year > 2037U) return ML307_RESULT_INVALID_VALUE;
  while (month <= 12U) {
    span = days[month - 1U];
    if (month == 2U && (year % 4U) == 0U) ++span;
    if (day_count < span) break;
    day_count -= span;
    ++month;
  }
  if (month > 12U) return ML307_RESULT_INVALID_VALUE;
  clock->year = year;
  clock->month = month;
  clock->day = (uint8_t)(day_count + 1U);
  clock->hour = (uint8_t)(seconds / 3600U);
  seconds %= 3600U;
  clock->minute = (uint8_t)(seconds / 60U);
  clock->second = (uint8_t)(seconds % 60U);
  clock->timezone_quarters = 0;
  return ML307_RESULT_OK;
}

static int64_t SslClockLinearSeconds(const ML307_ModuleClock *clock)
{
  static const uint8_t days[] = {31U, 28U, 31U, 30U, 31U, 30U,
                                 31U, 31U, 30U, 31U, 30U, 31U};
  uint16_t year;
  uint8_t month;
  int64_t day_count = 0;
  if (!SslClockIsValid(clock)) return -1;
  for (year = 1970U; year < clock->year; ++year)
    day_count += ((year % 4U) == 0U) ? 366 : 365;
  for (month = 1U; month < clock->month; ++month) {
    day_count += days[month - 1U];
    if (month == 2U && (clock->year % 4U) == 0U) ++day_count;
  }
  day_count += (int64_t)clock->day - 1;
  return day_count * 86400 + (int64_t)clock->hour * 3600 +
         (int64_t)clock->minute * 60 + clock->second -
         (int64_t)clock->timezone_quarters * 15 * 60;
}

void ML307_SslTransactionInit(ML307_SslTransaction *transaction)
{
  if (transaction != NULL) memset(transaction, 0, sizeof(*transaction));
}

static ML307_Result SslTransactionBegin(ML307_SslTransaction *transaction,
                                        const char *ca_name)
{
  if (transaction == NULL || !SslNameIsValid(ca_name, 0))
    return ML307_RESULT_INVALID_ARGUMENT;
  if (transaction->status == ML307_SSL_TRANSACTION_ACTIVE)
    return ML307_RESULT_INVALID_VALUE;
  memset(transaction, 0, sizeof(*transaction));
  memcpy(transaction->ca_name, ca_name, strlen(ca_name) + 1U);
  transaction->status = ML307_SSL_TRANSACTION_ACTIVE;
  return ML307_RESULT_OK;
}

ML307_Result ML307_SslTransactionStartCaInstall(
    ML307_SslTransaction *transaction, const char *ca_name,
    const uint8_t *certificate, size_t certificate_length)
{
  ML307_Result result;
  if (certificate == NULL || certificate_length == 0U ||
      certificate_length > ML307_SSL_CERTIFICATE_MAX)
    return ML307_RESULT_INVALID_VALUE;
  result = SslTransactionBegin(transaction, ca_name);
  if (result != ML307_RESULT_OK) return result;
  transaction->certificate = certificate;
  transaction->certificate_length = certificate_length;
  transaction->step = SSL_TX_SEND_CERT_WRITE;
  return ML307_RESULT_OK;
}

ML307_Result ML307_SslTransactionStartPrepare(
    ML307_SslTransaction *transaction, uint8_t ssl_id,
    uint8_t mqtt_connect_id, const char *ca_name,
    const ML307_ModuleClock *trusted_utc)
{
  ML307_Result result;
  if (ssl_id > ML307_SSL_CONTEXT_MAX || mqtt_connect_id > 5U ||
      !SslClockIsValid(trusted_utc) || trusted_utc->timezone_quarters != 0)
    return ML307_RESULT_INVALID_VALUE;
  result = SslTransactionBegin(transaction, ca_name);
  if (result != ML307_RESULT_OK) return result;
  transaction->ssl_id = ssl_id;
  transaction->mqtt_connect_id = mqtt_connect_id;
  transaction->clock = *trusted_utc;
  transaction->step = SSL_TX_SEND_OPTION_SET;
  return ML307_RESULT_OK;
}

ML307_Result ML307_SslTransactionGetAction(
    const ML307_SslTransaction *transaction, char *command,
    size_t command_capacity, ML307_SslAction *action)
{
  ML307_Result result = ML307_RESULT_INVALID_VALUE;
  ML307_SslCertificateChunk chunk;
  if (transaction == NULL || action == NULL ||
      transaction->status != ML307_SSL_TRANSACTION_ACTIVE)
    return ML307_RESULT_INVALID_ARGUMENT;
  memset(action, 0, sizeof(*action));
  if (transaction->step == SSL_TX_SEND_CERT_PAYLOAD) {
    action->kind = ML307_SSL_ACTION_RAW_PAYLOAD;
    action->data = transaction->certificate;
    action->length = transaction->certificate_length;
    return ML307_RESULT_OK;
  }
  if (command == NULL || command_capacity == 0U)
    return ML307_RESULT_INVALID_ARGUMENT;
  switch (transaction->step) {
  case SSL_TX_SEND_CERT_WRITE:
    chunk.name = transaction->ca_name;
    chunk.certificate = transaction->certificate;
    chunk.certificate_length = transaction->certificate_length;
    chunk.offset = 0U;
    chunk.length = transaction->certificate_length;
    result = ML307_SslBuildCertificateWrite(command, command_capacity, &chunk);
    break;
  case SSL_TX_SEND_CERT_READ:
    result = ML307_SslBuildCertificateRead(command, command_capacity,
                                            transaction->ca_name);
    break;
  case SSL_TX_SEND_OPTION_SET: {
    const ML307_SslOption option =
        s_prepare_options[transaction->option_index];
    result = ML307_SslBuildSetOption(command, command_capacity, option,
                                     transaction->ssl_id,
                                     SslPrepareValue(option));
    break;
  }
  case SSL_TX_SEND_OPTION_QUERY:
    result = ML307_SslBuildQueryOption(
        command, command_capacity,
        s_prepare_options[transaction->option_index], transaction->ssl_id);
    break;
  case SSL_TX_SEND_CA_SET:
    result = ML307_SslBuildSetCa(command, command_capacity,
                                 transaction->ssl_id, transaction->ca_name);
    break;
  case SSL_TX_SEND_CA_QUERY:
    result = ML307_SslBuildQueryOption(command, command_capacity,
                                       ML307_SSL_OPTION_CERTIFICATE,
                                       transaction->ssl_id);
    break;
  case SSL_TX_SEND_CLOCK_SET:
    result = ML307_SslBuildClockSet(command, command_capacity,
                                    &transaction->clock);
    break;
  case SSL_TX_SEND_CLOCK_QUERY:
    result = ML307_SslBuildClockQuery(command, command_capacity);
    break;
  case SSL_TX_SEND_MQTT_SET:
    result = ML307_MqttBuildSslConfig(
        command, command_capacity, transaction->mqtt_connect_id, 1U,
        transaction->ssl_id);
    break;
  case SSL_TX_SEND_MQTT_QUERY:
    result = ML307_MqttBuildSslQuery(command, command_capacity,
                                     transaction->mqtt_connect_id);
    break;
  default:
    return ML307_RESULT_INVALID_VALUE;
  }
  if (result != ML307_RESULT_OK) return result;
  action->kind = ML307_SSL_ACTION_AT_COMMAND;
  action->data = (const uint8_t *)command;
  action->length = strlen(command);
  return ML307_RESULT_OK;
}

ML307_Result ML307_SslTransactionActionSent(
    ML307_SslTransaction *transaction)
{
  if (transaction == NULL ||
      transaction->status != ML307_SSL_TRANSACTION_ACTIVE)
    return ML307_RESULT_INVALID_ARGUMENT;
  switch (transaction->step) {
  case SSL_TX_SEND_CERT_WRITE: transaction->step = SSL_TX_WAIT_CERT_PROMPT; break;
  case SSL_TX_SEND_CERT_PAYLOAD: transaction->step = SSL_TX_WAIT_CERT_FINAL; break;
  case SSL_TX_SEND_CERT_READ: transaction->step = SSL_TX_WAIT_CERT_READ; break;
  case SSL_TX_SEND_OPTION_SET: transaction->step = SSL_TX_WAIT_OPTION_SET; break;
  case SSL_TX_SEND_OPTION_QUERY: transaction->step = SSL_TX_WAIT_OPTION_QUERY; break;
  case SSL_TX_SEND_CA_SET: transaction->step = SSL_TX_WAIT_CA_SET; break;
  case SSL_TX_SEND_CA_QUERY: transaction->step = SSL_TX_WAIT_CA_QUERY; break;
  case SSL_TX_SEND_CLOCK_SET: transaction->step = SSL_TX_WAIT_CLOCK_SET; break;
  case SSL_TX_SEND_CLOCK_QUERY: transaction->step = SSL_TX_WAIT_CLOCK_QUERY; break;
  case SSL_TX_SEND_MQTT_SET: transaction->step = SSL_TX_WAIT_MQTT_SET; break;
  case SSL_TX_SEND_MQTT_QUERY: transaction->step = SSL_TX_WAIT_MQTT_QUERY; break;
  default: return ML307_RESULT_INVALID_VALUE;
  }
  return ML307_RESULT_OK;
}

static ML307_SslParseResult SslMqttParseResult(ML307_Result result)
{
  if (result == ML307_RESULT_OK) return ML307_SSL_PARSE_COMPLETE;
  if (result == ML307_RESULT_NOT_FOUND) return ML307_SSL_PARSE_INCOMPLETE;
  if (result == ML307_RESULT_ERROR_RESPONSE)
    return ML307_SSL_PARSE_ERROR_RESPONSE;
  return ML307_SSL_PARSE_INVALID;
}

ML307_SslParseResult ML307_SslTransactionInspectResponse(
    const ML307_SslTransaction *transaction, const uint8_t *response,
    size_t response_length, uint8_t *certificate_scratch,
    size_t certificate_capacity)
{
  ML307_SslOptionValue option_value;
  ML307_SslCertificateNames names;
  ML307_ModuleClock clock;
  size_t certificate_length = 0U;
  uint8_t mqtt_enable = 0U;
  uint8_t mqtt_ssl_id = 0U;
  ML307_SslParseResult result;
  if (transaction == NULL || response == NULL ||
      transaction->status != ML307_SSL_TRANSACTION_ACTIVE)
    return ML307_SSL_PARSE_INVALID;
  if (transaction->step != SSL_TX_WAIT_CERT_READ &&
      SslControlHasModuleReady(response, response_length) != 0U)
    return ML307_SSL_PARSE_MODULE_RESET;
  switch (transaction->step) {
  case SSL_TX_WAIT_CERT_PROMPT:
    return ML307_SslParseCertificateWritePrompt(response, response_length);
  case SSL_TX_WAIT_CERT_FINAL:
  case SSL_TX_WAIT_OPTION_SET:
  case SSL_TX_WAIT_CA_SET:
  case SSL_TX_WAIT_CLOCK_SET:
  case SSL_TX_WAIT_MQTT_SET:
    return ML307_SslParseFinalResult(response, response_length);
  case SSL_TX_WAIT_CERT_READ:
    if (certificate_scratch == NULL ||
        certificate_capacity < transaction->certificate_length)
      return ML307_SSL_PARSE_BUFFER_TOO_SMALL;
    result = ML307_SslParseCertificateRead(
        response, response_length, certificate_scratch, certificate_capacity,
        &certificate_length);
    if (result == ML307_SSL_PARSE_COMPLETE &&
        (certificate_length != transaction->certificate_length ||
         memcmp(certificate_scratch, transaction->certificate,
                certificate_length) != 0))
      return ML307_SSL_PARSE_INVALID;
    return result;
  case SSL_TX_WAIT_OPTION_QUERY:
    result = ML307_SslParseOptionQuery(
        response, response_length,
        s_prepare_options[transaction->option_index], transaction->ssl_id,
        &option_value);
    if (result == ML307_SSL_PARSE_COMPLETE &&
        option_value.value !=
            SslPrepareValue(s_prepare_options[transaction->option_index]))
      return ML307_SSL_PARSE_INVALID;
    return result;
  case SSL_TX_WAIT_CA_QUERY:
    result = ML307_SslParseCertificateQuery(
        response, response_length, transaction->ssl_id, &names);
    if (result == ML307_SSL_PARSE_COMPLETE &&
        (strcmp(names.server_ca, transaction->ca_name) != 0 ||
         names.client_certificate[0] != '\0' || names.private_key[0] != '\0'))
      return ML307_SSL_PARSE_INVALID;
    return result;
  case SSL_TX_WAIT_CLOCK_QUERY:
    result = ML307_SslParseClockQuery(response, response_length, &clock);
    if (result == ML307_SSL_PARSE_COMPLETE) {
      const int64_t expected = SslClockLinearSeconds(&transaction->clock);
      const int64_t actual = SslClockLinearSeconds(&clock);
      const int64_t delta = actual - expected;
      if (clock.timezone_quarters != 0 || expected < 0 || actual < 0 ||
          delta < 0 || delta > 5)
        return ML307_SSL_PARSE_INVALID;
    }
    return result;
  case SSL_TX_WAIT_MQTT_QUERY:
    result = SslMqttParseResult(ML307_MqttParseSslQuery(
        response, response_length, transaction->mqtt_connect_id,
        &mqtt_enable, &mqtt_ssl_id));
    if (result == ML307_SSL_PARSE_COMPLETE &&
        (mqtt_enable != 1U || mqtt_ssl_id != transaction->ssl_id))
      return ML307_SSL_PARSE_INVALID;
    return result;
  default:
    return ML307_SSL_PARSE_INVALID;
  }
}

ML307_SslParseResult ML307_SslTransactionConsumeResponse(
    ML307_SslTransaction *transaction, const uint8_t *response,
    size_t response_length, uint8_t *certificate_scratch,
    size_t certificate_capacity)
{
  ML307_SslParseResult result = ML307_SslTransactionInspectResponse(
      transaction, response, response_length, certificate_scratch,
      certificate_capacity);
  if (result == ML307_SSL_PARSE_INCOMPLETE) return result;
  if (result != ML307_SSL_PARSE_COMPLETE) {
    if (transaction != NULL)
      transaction->status = ML307_SSL_TRANSACTION_FAILED;
    return result;
  }
  switch (transaction->step) {
  case SSL_TX_WAIT_CERT_PROMPT:
    transaction->step = SSL_TX_SEND_CERT_PAYLOAD;
    break;
  case SSL_TX_WAIT_CERT_FINAL:
    transaction->step = SSL_TX_SEND_CERT_READ;
    break;
  case SSL_TX_WAIT_CERT_READ:
    transaction->status = ML307_SSL_TRANSACTION_COMPLETE;
    break;
  case SSL_TX_WAIT_OPTION_SET:
    transaction->step = SSL_TX_SEND_OPTION_QUERY;
    break;
  case SSL_TX_WAIT_OPTION_QUERY:
    ++transaction->option_index;
    transaction->step =
        (transaction->option_index <
         (uint8_t)(sizeof(s_prepare_options) / sizeof(s_prepare_options[0])))
            ? SSL_TX_SEND_OPTION_SET
            : SSL_TX_SEND_CA_SET;
    break;
  case SSL_TX_WAIT_CA_SET:
    transaction->step = SSL_TX_SEND_CA_QUERY;
    break;
  case SSL_TX_WAIT_CA_QUERY:
    transaction->step = SSL_TX_SEND_CLOCK_SET;
    break;
  case SSL_TX_WAIT_CLOCK_SET:
    transaction->step = SSL_TX_SEND_CLOCK_QUERY;
    break;
  case SSL_TX_WAIT_CLOCK_QUERY:
    transaction->step = SSL_TX_SEND_MQTT_SET;
    break;
  case SSL_TX_WAIT_MQTT_SET:
    transaction->step = SSL_TX_SEND_MQTT_QUERY;
    break;
  case SSL_TX_WAIT_MQTT_QUERY:
    transaction->status = ML307_SSL_TRANSACTION_COMPLETE;
    break;
  default:
    transaction->status = ML307_SSL_TRANSACTION_FAILED;
    return ML307_SSL_PARSE_INVALID;
  }
  return ML307_SSL_PARSE_COMPLETE;
}

void ML307_SslTransactionCancel(ML307_SslTransaction *transaction)
{
  if (transaction != NULL) memset(transaction, 0, sizeof(*transaction));
}

ML307_SslTransactionStatus ML307_SslTransactionGetStatus(
    const ML307_SslTransaction *transaction)
{
  if (transaction == NULL) return ML307_SSL_TRANSACTION_FAILED;
  return (ML307_SslTransactionStatus)transaction->status;
}
