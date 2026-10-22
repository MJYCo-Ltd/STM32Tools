#include <AT/ModuleFrameParser.h>

#include <string.h>

uint8_t ModuleFrameParser_NextLine(const uint8_t *data, size_t length,
                                   size_t *offset, const uint8_t **line,
                                   size_t *line_length)
{
  size_t begin;
  size_t end;
  size_t cursor;
  if ((data == NULL) || (offset == NULL) || (line == NULL) ||
      (line_length == NULL) || (*offset >= length)) return 0U;
  begin = *offset;
  for (cursor = begin; cursor < length; ++cursor) {
    if (data[cursor] != (uint8_t)'\n') continue;
    end = cursor;
    if ((end > begin) && (data[end - 1U] == (uint8_t)'\r')) --end;
    *line = &data[begin];
    *line_length = end - begin;
    *offset = cursor + 1U;
    return 1U;
  }
  return 0U;
}

const uint8_t *ModuleFrameParser_FindLinePrefix(const uint8_t *data,
                                                size_t length,
                                                const char *prefix)
{
  size_t line_start = 0U;
  const size_t prefix_length = (prefix != NULL) ? strlen(prefix) : 0U;
  if ((data == NULL) || (prefix_length == 0U)) return NULL;
  while (line_start < length) {
    size_t cursor;
    if (((length - line_start) >= prefix_length) &&
        (memcmp(&data[line_start], prefix, prefix_length) == 0)) {
      return &data[line_start];
    }
    for (cursor = line_start; cursor < length; ++cursor) {
      if (data[cursor] == (uint8_t)'\n') break;
    }
    if (cursor >= length) break;
    line_start = cursor + 1U;
  }
  return NULL;
}

uint8_t ModuleFrameParser_HasLine(const uint8_t *data, size_t length,
                                  const char *value, uint8_t prefix_match)
{
  size_t offset = 0U;
  const uint8_t *line;
  size_t line_length;
  const size_t value_length = (value != NULL) ? strlen(value) : 0U;
  if (value_length == 0U) return 0U;
  while (ModuleFrameParser_NextLine(data, length, &offset, &line,
                                    &line_length) != 0U) {
    if (((prefix_match != 0U) && (line_length >= value_length) &&
         (memcmp(line, value, value_length) == 0)) ||
        ((prefix_match == 0U) && (line_length == value_length) &&
         (memcmp(line, value, value_length) == 0))) {
      return 1U;
    }
  }
  return 0U;
}

ModuleFrameResult ModuleFrameParser_ParseUnsigned(const uint8_t **cursor,
                                                   const uint8_t *end,
                                                   uint32_t maximum,
                                                   uint32_t *value)
{
  uint32_t parsed = 0U;
  uint8_t have_digit = 0U;
  if ((cursor == NULL) || (*cursor == NULL) || (end == NULL) ||
      (value == NULL) || (*cursor > end)) return MODULE_FRAME_INVALID;
  while ((*cursor < end) && (**cursor >= (uint8_t)'0') &&
         (**cursor <= (uint8_t)'9')) {
    const uint32_t digit = (uint32_t)(**cursor - (uint8_t)'0');
    if ((digit > maximum) ||
        (parsed > ((maximum - digit) / 10U))) return MODULE_FRAME_INVALID;
    parsed = (parsed * 10U) + digit;
    have_digit = 1U;
    ++(*cursor);
  }
  if (have_digit == 0U) {
    return (*cursor >= end) ? MODULE_FRAME_INCOMPLETE : MODULE_FRAME_INVALID;
  }
  *value = parsed;
  return MODULE_FRAME_COMPLETE;
}

void ModuleFrameParser_InitLineCollector(ModuleLineCollector *collector,
                                         char *buffer, size_t capacity,
                                         ModuleFrameLineCallback callback,
                                         void *context)
{
  if (collector == NULL) return;
  collector->buffer = buffer;
  collector->capacity = capacity;
  collector->length = 0U;
  collector->discarding = 0U;
  collector->preserve_terminator = 0U;
  collector->callback = callback;
  collector->context = context;
  if ((buffer != NULL) && (capacity > 0U)) buffer[0] = '\0';
}

static void FeedLines(ModuleLineCollector *collector, const uint8_t *data,
                      size_t length, uint8_t keep_ending)
{
  size_t i;
  if (collector == NULL || collector->buffer == NULL ||
      collector->capacity < 2U || data == NULL) return;
  for (i = 0U; i < length; ++i) {
    const uint8_t byte = data[i];
    if (collector->discarding) {
      if (byte == '\n') collector->discarding = 0U;
      continue;
    }
    /* Text records may not hide a suffix behind an embedded C terminator. */
    if (byte == 0U || (collector->length >= collector->capacity - 1U &&
                       (byte != '\n' || keep_ending))) {
      collector->length = 0U;
      collector->buffer[0] = '\0';
      collector->discarding = byte != '\n';
      continue;
    }
    if (byte != '\n' || keep_ending) {
      collector->buffer[collector->length++] = (char)byte;
    }
    if (byte == '\n') {
      size_t completed = collector->length;
      if (!keep_ending) {
        while (completed && collector->buffer[completed - 1U] == '\r') --completed;
      }
      collector->buffer[completed] = '\0';
      /* Clear assembly state before dispatch. Buffer is borrowed until callback
       * returns; recursive Feed on the same collector is not supported. */
      collector->length = 0U;
      if (collector->callback) collector->callback(collector->buffer, completed,
                                                    collector->context);
    }
  }
}

void ModuleFrameParser_FeedLines(ModuleLineCollector *collector,
                                 const uint8_t *data, size_t length)
{
  FeedLines(collector, data, length, collector ? collector->preserve_terminator : 0U);
}

void ModuleFrameParser_FeedRawLines(ModuleLineCollector *collector,
                                    const uint8_t *data, size_t length)
{
  FeedLines(collector, data, length, 1U);
}

ModuleFrameResult ModuleFrameParser_ParseLengthHeader(
    const uint8_t *data, size_t length, const ModuleLengthFrameProtocolEx *protocol,
    uint32_t *fields, size_t fields_capacity, ModuleFrameSpan *quoted,
    const uint8_t **payload, size_t *payload_length)
{
  const uint8_t *cursor, *end;
  size_t i, prefix_length;
  ModuleFrameSpan parsed_quote = {0};
  if (payload != NULL) *payload = NULL;
  if (payload_length != NULL) *payload_length = 0U;
  if (quoted != NULL) *quoted = (ModuleFrameSpan){0};
  if (data == NULL || protocol == NULL || protocol->base.prefix == NULL ||
      !protocol->base.field_count || protocol->base.payload_length_field >= protocol->base.field_count ||
      protocol->quoted_field > protocol->base.field_count ||
      protocol->quoted_field == protocol->base.payload_length_field + 1U ||
      fields == NULL || fields_capacity < protocol->base.field_count ||
      payload == NULL || payload_length == NULL) return MODULE_FRAME_INVALID;
  prefix_length = strlen(protocol->base.prefix);
  if (memcmp(data, protocol->base.prefix, length < prefix_length ? length : prefix_length))
    return MODULE_FRAME_INVALID;
  if (length < prefix_length) return MODULE_FRAME_INCOMPLETE;
  cursor = data + prefix_length;
  end = data + length;
  /* Limit header parsing independently of the opaque body. */
  if (protocol->maximum_header_length && length > protocol->maximum_header_length)
    end = data + protocol->maximum_header_length;
  while (cursor < end && (*cursor == ' ' || *cursor == '\t')) ++cursor;
  for (i = 0U; i < protocol->base.field_count; ++i) {
    if (protocol->quoted_field == i + 1U) {
      const uint8_t *begin;
      if (cursor == end) goto incomplete;
      if (*cursor++ != '"') return MODULE_FRAME_INVALID;
      begin = cursor;
      while (cursor < end && *cursor != '"') {
        if (*cursor == 0U || *cursor == '\r' || *cursor == '\n') return MODULE_FRAME_INVALID;
        ++cursor;
        if (protocol->maximum_quoted_length &&
            (size_t)(cursor - begin) > protocol->maximum_quoted_length)
          return MODULE_FRAME_INVALID;
      }
      if (cursor == end) goto incomplete;
      if (cursor == begin) return MODULE_FRAME_INVALID;
      parsed_quote = (ModuleFrameSpan){begin, (size_t)(cursor - begin)};
      fields[i] = 0U;
      ++cursor;
    } else {
      ModuleFrameResult result = ModuleFrameParser_ParseUnsigned(&cursor, end, UINT32_MAX, &fields[i]);
      if (result == MODULE_FRAME_INVALID) return result;
      if (result == MODULE_FRAME_INCOMPLETE) goto incomplete;
    }
    if (cursor == end) goto incomplete;
    if (*cursor != protocol->base.field_separator &&
        (i + 1U != protocol->base.field_count || !protocol->base.alternate_payload_separator ||
         *cursor != protocol->base.alternate_payload_separator)) return MODULE_FRAME_INVALID;
    ++cursor;
  }
  if (quoted != NULL) *quoted = parsed_quote;
  *payload = cursor;
  *payload_length = fields[protocol->base.payload_length_field];
  return MODULE_FRAME_COMPLETE;
incomplete:
  return protocol->maximum_header_length && length >= protocol->maximum_header_length
             ? MODULE_FRAME_INVALID : MODULE_FRAME_INCOMPLETE;
}

ModuleFrameResult ModuleFrameParser_ParseLengthFrameEx(
    const uint8_t *data, size_t length, const ModuleLengthFrameProtocolEx *protocol,
    uint32_t *fields, size_t fields_capacity, ModuleFrameSpan *quoted,
    const uint8_t **payload, size_t *payload_length, size_t *consumed)
{
  const uint8_t *begin, *body;
  size_t body_length, tail_consumed = 0U;
  ModuleFrameResult result;
  if (consumed != NULL) *consumed = 0U;
  if (data == NULL || protocol == NULL || consumed == NULL ||
      payload == NULL || payload_length == NULL ||
      (protocol->base.parse_tail == NULL &&
       (protocol->base.fixed_tail == NULL || !protocol->base.fixed_tail_length))) return MODULE_FRAME_INVALID;
  begin = ModuleFrameParser_FindLinePrefix(data, length, protocol->base.prefix);
  if (begin == NULL) return MODULE_FRAME_INCOMPLETE;
  result = ModuleFrameParser_ParseLengthHeader(begin, length - (size_t)(begin - data),
      protocol, fields, fields_capacity, quoted, &body, &body_length);
  if (result != MODULE_FRAME_COMPLETE) return result;
  *payload = body;
  *payload_length = body_length;
  if (body_length > protocol->base.maximum_payload_length) return MODULE_FRAME_INVALID;
  if (body_length > length - (size_t)(body - data)) return MODULE_FRAME_INCOMPLETE;
  body += body_length;
  if (protocol->base.parse_tail != NULL) {
    result = protocol->base.parse_tail(body, length - (size_t)(body - data), &tail_consumed);
    if (result != MODULE_FRAME_COMPLETE) return result;
  } else {
    size_t available = length - (size_t)(body - data);
    size_t compare = available < protocol->base.fixed_tail_length ? available : protocol->base.fixed_tail_length;
    if (memcmp(body, protocol->base.fixed_tail, compare)) return MODULE_FRAME_INVALID;
    if (available < protocol->base.fixed_tail_length) return MODULE_FRAME_INCOMPLETE;
    tail_consumed = protocol->base.fixed_tail_length;
  }
  if (tail_consumed > length - (size_t)(body - data)) return MODULE_FRAME_INVALID;
  *consumed = (size_t)(body - data) + tail_consumed;
  return MODULE_FRAME_COMPLETE;
}

ModuleFrameResult ModuleFrameParser_ParseLengthFrame(
    const uint8_t *data, size_t length, const ModuleLengthFrameProtocol *protocol,
    uint32_t *fields, size_t fields_capacity,
    const uint8_t **payload, size_t *payload_length, size_t *consumed)
{
  ModuleLengthFrameProtocolEx extended;
  if (protocol == NULL) return MODULE_FRAME_INVALID;
  extended = (ModuleLengthFrameProtocolEx){.base = *protocol};
  return ModuleFrameParser_ParseLengthFrameEx(data, length, &extended, fields,
      fields_capacity, NULL, payload, payload_length, consumed);
}
