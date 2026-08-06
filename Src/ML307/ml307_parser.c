/*
 ******************************************************************************
 * @file           : ml307_parser.c
 * @brief          : ML307 AT response parser (V2.0.5)
 ******************************************************************************
 */
#include "ML307/ml307_parser.h"

#include <stddef.h>
#include <string.h>

static size_t ML307_LineLength(const char *line)
{
  size_t length = 0U;

  while ((line[length] != '\0') && (line[length] != '\r') &&
         (line[length] != '\n')) {
    ++length;
  }
  return length;
}

static const char *ML307_NextLine(const char *line, size_t length)
{
  const char *next = line + length;

  while ((*next == '\r') || (*next == '\n')) {
    ++next;
  }
  return next;
}

static void ML307_TrimLine(const char **line, size_t *length)
{
  while ((*length > 0U) && ((**line == ' ') || (**line == '\t'))) {
    ++(*line);
    --(*length);
  }
  while ((*length > 0U) &&
         (((*line)[*length - 1U] == ' ') ||
          ((*line)[*length - 1U] == '\t'))) {
    --(*length);
  }
}

static int ML307_LineEquals(const char *line, size_t length, const char *token)
{
  const size_t token_length = strlen(token);
  return (length == token_length) && (strncmp(line, token, length) == 0);
}

static int ML307_LineStartsWith(const char *line, size_t length,
                                const char *prefix)
{
  const size_t prefix_length = strlen(prefix);
  return (length >= prefix_length) &&
         (strncmp(line, prefix, prefix_length) == 0);
}

static void ML307_Copy(char *destination, size_t destination_size,
                       const char *source, size_t source_length,
                       uint8_t *truncated)
{
  size_t copy_length = source_length;

  if (copy_length >= destination_size) {
    copy_length = destination_size - 1U;
    *truncated = 1U;
  }
  if (copy_length > 0U) {
    memcpy(destination, source, copy_length);
  }
  destination[copy_length] = '\0';
}

static void ML307_AppendLine(ML307_ParsedResponse *out, const char *line,
                             size_t length)
{
  size_t used = strlen(out->info);
  size_t available;
  size_t copy_length;

  if (length == 0U) {
    return;
  }
  if (used >= (sizeof(out->info) - 1U)) {
    out->truncated = 1U;
    return;
  }
  if (used > 0U) {
    out->info[used++] = '\n';
    out->info[used] = '\0';
  }
  available = sizeof(out->info) - used - 1U;
  copy_length = length;
  if (copy_length > available) {
    copy_length = available;
    out->truncated = 1U;
  }
  if (copy_length > 0U) {
    memcpy(&out->info[used], line, copy_length);
    out->info[used + copy_length] = '\0';
  }
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
  const char *cursor;

  if (raw == NULL) {
    return 0;
  }
  cursor = raw;
  while (*cursor != '\0') {
    const char *line = cursor;
    size_t length = ML307_LineLength(line);
    cursor = ML307_NextLine(line, length);
    ML307_TrimLine(&line, &length);

    if (ML307_LineEquals(line, length, "OK") ||
        ML307_LineEquals(line, length, "ERROR") ||
        ML307_LineStartsWith(line, length, "+CME ERROR:") ||
        ML307_LineStartsWith(line, length, "+CMS ERROR:")) {
      return 1;
    }
  }
  return 0;
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
    const char *line = cursor;
    size_t length = ML307_LineLength(line);
    const char *colon;

    cursor = ML307_NextLine(line, length);
    ML307_TrimLine(&line, &length);
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
