#include <Common.h>
#include <Protocol/HttpRange.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int HeaderNameEquals(const uint8_t *line, size_t name_length,
                            const char *name)
{
  size_t i;
  if (strlen(name) != name_length) return 0;
  for (i = 0U; i < name_length; ++i) {
    if (tolower((int)line[i]) != tolower((int)(uint8_t)name[i])) return 0;
  }
  return 1;
}

static uint8_t ParseUnsigned(const uint8_t **cursor, const uint8_t *end,
                             uint32_t *value)
{
  uint32_t parsed = 0U;
  uint8_t have_digit = 0U;
  while ((*cursor < end) && (**cursor >= (uint8_t)'0') &&
         (**cursor <= (uint8_t)'9')) {
    const uint32_t digit = (uint32_t)(**cursor - (uint8_t)'0');
    if (parsed > ((UINT32_MAX - digit) / 10U)) return 0U;
    parsed = (parsed * 10U) + digit;
    have_digit = 1U;
    ++(*cursor);
  }
  if (have_digit == 0U) return 0U;
  *value = parsed;
  return 1U;
}

uint8_t HttpRange_ParseUrl(const char *url, HttpRangeUrl *parsed)
{
  const char *authority;
  const char *slash;
  const char *colon = NULL;
  const char *cursor;
  size_t scheme_length;
  size_t host_length;
  size_t origin_length;
  size_t path_length;
  if ((url == NULL) || (parsed == NULL)) return 0U;
  memset(parsed, 0, sizeof(*parsed));
  if (strncmp(url, "https://", 8U) == 0) {
    scheme_length = 8U;
    parsed->https = 1U;
    parsed->port = 443U;
  } else if (strncmp(url, "http://", 7U) == 0) {
    scheme_length = 7U;
    parsed->https = 0U;
    parsed->port = 80U;
  } else {
    return 0U;
  }
  authority = url + scheme_length;
  slash = strchr(authority, '/');
  if (slash == NULL) slash = authority + strlen(authority);
  for (cursor = authority; cursor < slash; ++cursor) {
    if (*cursor == ':') colon = cursor;
  }
  host_length = (size_t)(((colon != NULL) ? colon : slash) - authority);
  origin_length = (size_t)(slash - url);
  path_length = (*slash == '\0') ? 1U : strlen(slash);
  if ((host_length == 0U) || (host_length >= sizeof(parsed->host)) ||
      (origin_length >= sizeof(parsed->origin)) ||
      (path_length >= sizeof(parsed->path))) return 0U;
  memcpy(parsed->host, authority, host_length);
  parsed->host[host_length] = '\0';
  memcpy(parsed->origin, url, origin_length);
  parsed->origin[origin_length] = '\0';
  if (*slash == '\0') {
    (void)snprintf(parsed->path, sizeof(parsed->path), "/");
  } else {
    memcpy(parsed->path, slash, path_length + 1U);
  }
  if (colon != NULL) {
    char *end = NULL;
    const unsigned long port = strtoul(colon + 1, &end, 10);
    if ((end != slash) || (port == 0U) || (port > UINT16_MAX)) return 0U;
    parsed->port = (uint16_t)port;
  }
  return 1U;
}

uint8_t HttpRange_BuildGet(char *output, size_t capacity,
                                   const char *path, const char *host,
                                   uint32_t range_start, uint32_t range_end)
{
  int count;
  if ((output == NULL) || (capacity == 0U) || (path == NULL) ||
      (host == NULL) || (range_end < range_start)) return 0U;
  count = snprintf(output, capacity,
                   "GET %s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\n"
                   "Connection: keep-alive\r\nRange: bytes=%lu-%lu\r\n\r\n",
                   path, host, (unsigned long)range_start,
                   (unsigned long)range_end);
  return ((count > 0) && ((size_t)count < capacity)) ? 1U : 0U;
}

uint8_t HttpRange_ParseHeader(const uint8_t *header, size_t length,
                                      uint32_t expected_start,
                                      uint32_t expected_length,
                                      uint32_t expected_total,
                                      HttpRangeHeaders *metadata, HttpRangeHeaderFn extra, void *ctx)
{
  const uint8_t *cursor;
  const uint8_t *limit;
  HttpRangeHeaders parsed;
  uint8_t got_length = 0U;
  uint8_t got_range = 0U;
  if ((header == NULL) || (metadata == NULL) || (expected_length == 0U) ||
      (expected_start >= expected_total) ||
      (expected_length > expected_total - expected_start)) {
    return 0U;
  }
  cursor = header;
  limit = header + length;
  memset(&parsed, 0, sizeof(parsed));
  while (cursor < limit) {
    const uint8_t *line_end = FindBytes(cursor, (size_t)(limit - cursor), "\r\n");
    const uint8_t *colon;
    const uint8_t *value;
    if (line_end == NULL) line_end = limit;
    colon = (const uint8_t *)memchr(cursor, ':', (size_t)(line_end - cursor));
    if (colon == NULL) {
      cursor = line_end + ((line_end < limit) ? 2U : 0U);
      continue;
    }
    if (HeaderNameEquals(cursor, (size_t)(colon - cursor),
                         "Transfer-Encoding")) return 0U;
    value = colon + 1U;
    while ((value < line_end) && ((*value == (uint8_t)' ') ||
                                  (*value == (uint8_t)'\t'))) ++value;
    if (HeaderNameEquals(cursor, (size_t)(colon - cursor), "Content-Length")) {
      const uint8_t *end = value;
      if (got_length != 0U) return 0U;
      if ((ParseUnsigned(&end, line_end, &parsed.content_length) == 0U) ||
          (end != line_end)) return 0U;
      got_length = 1U;
    } else if (HeaderNameEquals(cursor, (size_t)(colon - cursor),
                                "Content-Range")) {
      static const char unit[] = "bytes ";
      if (got_range != 0U) return 0U;
      if (((size_t)(line_end - value) < (sizeof(unit) - 1U)) ||
          (memcmp(value, unit, sizeof(unit) - 1U) != 0)) return 0U;
      value += sizeof(unit) - 1U;
      if ((ParseUnsigned(&value, line_end, &parsed.range_start) == 0U) ||
          (value >= line_end) || (*value++ != (uint8_t)'-') ||
          (ParseUnsigned(&value, line_end, &parsed.range_end) == 0U) ||
          (value >= line_end) || (*value++ != (uint8_t)'/') ||
          (ParseUnsigned(&value, line_end, &parsed.total_length) == 0U) ||
          (value != line_end)) return 0U;
      got_range = 1U;
    } else if ((extra != NULL) &&
               (extra(ctx, cursor, (size_t)(colon - cursor), value,
                      (size_t)(line_end - value)) == 0U)) {
      return 0U;
    }
    cursor = line_end + ((line_end < limit) ? 2U : 0U);
  }
  if ((got_length == 0U) || (got_range == 0U) ||
      (parsed.content_length != expected_length) ||
      (parsed.range_start != expected_start) ||
      (parsed.range_end != (expected_start + expected_length - 1U)) ||
      (parsed.total_length != expected_total)) return 0U;
  *metadata = parsed;
  return 1U;
}

int HttpRange_ParseResponse(const uint8_t *response, size_t length,
                                    uint32_t expected_start,
                                    uint32_t expected_length,
                                    uint32_t expected_total,
                                    size_t *body_offset, HttpRangeHeaders *metadata,
                                    HttpRangeHeaderFn extra, void *ctx)
{
  const uint8_t *header_end;
  HttpRangeHeaders parsed;
  size_t offset;
  if ((response == NULL) || (body_offset == NULL) || (metadata == NULL)) return -1;
  header_end = FindBytes(response, length, "\r\n\r\n");
  if (header_end == NULL) return 0;
  /* Include the complete status code and its following space. Derive the
   * length from the literal so the final '6' cannot be mistaken for space. */
  static const char partial_11[] = "HTTP/1.1 206 ";
  static const char partial_10[] = "HTTP/1.0 206 ";
  if ((length < sizeof(partial_11) - 1U) ||
      ((memcmp(response, partial_11, sizeof(partial_11) - 1U) != 0) &&
       (memcmp(response, partial_10, sizeof(partial_10) - 1U) != 0))) return -1;
  if (HttpRange_ParseHeader(
          response, (size_t)(header_end - response), expected_start,
          expected_length, expected_total, &parsed, extra, ctx) == 0U) return -1;
  offset = (size_t)((header_end + 4U) - response);
  if ((offset > length) ||
      (parsed.content_length > (length - offset))) return 0;
  *metadata = parsed;
  *body_offset = offset;
  return 1;
}
