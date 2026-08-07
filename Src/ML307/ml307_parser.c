/*
 ******************************************************************************
 * @file           : ml307_parser.c
 * @brief          : ML307 AT response parser (V2.0.5)
 ******************************************************************************
 */
#include "ML307/ml307_parser.h"

#include "AT/at_codec.h"

#include <stddef.h>
#include <string.h>

static void ML307_TrimLine(const char **line, size_t *length)
{
  AT_Line value = {*line, *length};
  AT_TrimLine(&value);
  *line = value.data;
  *length = value.length;
}

static int ML307_LineEquals(const char *line, size_t length, const char *token)
{
  const AT_Line value = {line, length};
  return AT_LineEquals(&value, token);
}

static int ML307_LineStartsWith(const char *line, size_t length,
                                const char *prefix)
{
  const AT_Line value = {line, length};
  return AT_LineStartsWith(&value, prefix);
}

static void ML307_Copy(char *destination, size_t destination_size,
                       const char *source, size_t source_length,
                       uint8_t *truncated)
{
  AT_CopyText(destination, destination_size, source, source_length, truncated);
}

static void ML307_AppendLine(ML307_ParsedResponse *out, const char *line,
                             size_t length)
{
  AT_AppendLine(out->info, sizeof(out->info), line, length, &out->truncated);
}

static int ML307_IsEchoLine(const char *line, size_t length)
{
  if (ML307_LineStartsWith(line, length, "AT") ||
      ML307_LineEquals(line, length, "ATI") ||
      ML307_LineEquals(line, length, "ATA") ||
      ML307_LineEquals(line, length, "ATH") ||
      ML307_LineStartsWith(line, length, "ATD") ||
      ML307_LineStartsWith(line, length, "ATE") ||
      ML307_LineStartsWith(line, length, "ATS") ||
      ML307_LineStartsWith(line, length, "AT&") ||
      ML307_LineStartsWith(line, length, "ATV") ||
      ML307_LineStartsWith(line, length, "ATQ") ||
      ML307_LineStartsWith(line, length, "ATZ") ||
      ML307_LineStartsWith(line, length, "ATX")) {
    return 1;
  }
  return 0;
}

static int ML307_IsPlainExpected(const char *expected_type)
{
  static const char *const plain[] = {
      "ATI",  "GMI",  "CGMI", "GMM",  "CGMM", "GMR",  "CGMR",
      "GSN",  "CGSN", "CIMI", "CLAC", "ATS0", "ATS3", "ATS4",
      "ATS5", NULL};
  size_t i;

  for (i = 0U; plain[i] != NULL; ++i) {
    if (strcmp(expected_type, plain[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

int ML307_ResponseIsComplete(const char *raw)
{
  return AT_HasFinalResult(raw);
}

ML307_ParseResult ML307_ParseResponse(const char *raw,
                                      const char *expected_type,
                                      ML307_ParsedResponse *out)
{
  const char *cursor;
  const size_t expected_length =
      (expected_type != NULL) ? strlen(expected_type) : 0U;
  const int plain_mode =
      ((expected_type != NULL) && (ML307_IsPlainExpected(expected_type) != 0));
  int found_target = 0;
  int collecting_target = 0;

  if ((raw == NULL) || (expected_type == NULL) ||
      (expected_length == 0U) || (out == NULL)) {
    return ML307_PARSE_INVALID_ARGUMENT;
  }
  memset(out, 0, sizeof(*out));
  cursor = raw;

  while (*cursor != '\0') {
    AT_Line parsed_line;
    const char *line;
    size_t length;
    const char *colon;

    cursor = AT_ReadLine(cursor, &parsed_line);
    line = parsed_line.data;
    length = parsed_line.length;
    if (length == 0U) {
      continue;
    }
    if (ML307_LineEquals(line, length, "OK")) {
      out->has_ok = 1U;
      collecting_target = 0;
      continue;
    }
    if (ML307_LineEquals(line, length, "ERROR")) {
      out->has_error = 1U;
      ML307_Copy(out->type, sizeof(out->type), "ERROR", 5U, &out->truncated);
      collecting_target = 0;
      continue;
    }
    if (ML307_LineStartsWith(line, length, "+CME ERROR:")) {
      const char *payload;
      size_t payload_length;

      out->has_error = 1U;
      ML307_Copy(out->type, sizeof(out->type), "CME ERROR", 9U,
                 &out->truncated);
      colon = (const char *)memchr(line, ':', length);
      payload = colon + 1;
      payload_length = (size_t)((line + length) - payload);
      ML307_TrimLine(&payload, &payload_length);
      out->info[0] = '\0';
      ML307_AppendLine(out, payload, payload_length);
      collecting_target = 0;
      continue;
    }
    if (ML307_LineStartsWith(line, length, "+CMS ERROR:")) {
      const char *payload;
      size_t payload_length;

      out->has_error = 1U;
      ML307_Copy(out->type, sizeof(out->type), "CMS ERROR", 9U,
                 &out->truncated);
      colon = (const char *)memchr(line, ':', length);
      payload = colon + 1;
      payload_length = (size_t)((line + length) - payload);
      ML307_TrimLine(&payload, &payload_length);
      out->info[0] = '\0';
      ML307_AppendLine(out, payload, payload_length);
      collecting_target = 0;
      continue;
    }
    if (ML307_IsEchoLine(line, length) != 0) {
      continue;
    }

    if ((line[0] == '+') && (length > 1U)) {
      size_t type_length;
      const char *payload;
      size_t payload_length;

      colon = (const char *)memchr(line + 1, ':', length - 1U);
      type_length = (colon != NULL) ? (size_t)(colon - (line + 1))
                                    : (length - 1U);
      if ((type_length == expected_length) &&
          (strncmp(line + 1, expected_type, expected_length) == 0)) {
        if (found_target == 0) {
          ML307_Copy(out->type, sizeof(out->type), expected_type,
                     expected_length, &out->truncated);
        }
        found_target = 1;
        collecting_target = 1;
        if (colon != NULL) {
          payload = colon + 1;
          payload_length = (size_t)((line + length) - payload);
          ML307_TrimLine(&payload, &payload_length);
          ML307_AppendLine(out, payload, payload_length);
        }
      } else {
        collecting_target = 0;
      }
      continue;
    }

    if (plain_mode != 0) {
      if (found_target == 0) {
        ML307_Copy(out->type, sizeof(out->type), expected_type,
                   expected_length, &out->truncated);
      }
      found_target = 1;
      collecting_target = 1;
      ML307_AppendLine(out, line, length);
    } else if (collecting_target != 0) {
      ML307_AppendLine(out, line, length);
    }
  }

  if (out->has_error != 0U) {
    return ML307_PARSE_ERROR_RESPONSE;
  }
  if (found_target == 0) {
    /* Query that only returns OK (e.g. some SETs) is not used here; treat as
     * not found so the app can show a clear error. */
    if ((out->has_ok != 0U) && (plain_mode == 0)) {
      return ML307_PARSE_NOT_FOUND;
    }
    return ML307_PARSE_NOT_FOUND;
  }
  if (out->truncated != 0U) {
    return ML307_PARSE_TRUNCATED;
  }
  return ML307_PARSE_OK;
}
