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

ModuleFrameResult ModuleFrameParser_ParseLengthFrame(
    const uint8_t *data, size_t length,
    const ModuleLengthFrameProtocol *protocol,
    uint32_t *fields, size_t fields_capacity,
    const uint8_t **payload, size_t *payload_length, size_t *consumed)
{
  const uint8_t *cursor;
  const uint8_t *end;
  size_t field_index;
  size_t body_length;
  size_t tail_consumed = 0U;
  ModuleFrameResult result;
  if ((data == NULL) || (protocol == NULL) || (protocol->prefix == NULL) ||
      (protocol->field_count == 0U) ||
      (protocol->payload_length_field >= protocol->field_count) ||
      (fields == NULL) || (fields_capacity < protocol->field_count) ||
      (payload == NULL) || (payload_length == NULL) || (consumed == NULL) ||
      ((protocol->parse_tail == NULL) &&
       ((protocol->fixed_tail == NULL) ||
        (protocol->fixed_tail_length == 0U)))) return MODULE_FRAME_INVALID;

  cursor = ModuleFrameParser_FindLinePrefix(data, length, protocol->prefix);
  if (cursor == NULL) return MODULE_FRAME_INCOMPLETE;
  end = data + length;
  cursor += strlen(protocol->prefix);
  while ((cursor < end) && ((*cursor == (uint8_t)' ') ||
                            (*cursor == (uint8_t)'\t'))) ++cursor;

  for (field_index = 0U; field_index < protocol->field_count; ++field_index) {
    result = ModuleFrameParser_ParseUnsigned(&cursor, end, UINT32_MAX,
                                             &fields[field_index]);
    if (result != MODULE_FRAME_COMPLETE) return result;
    if (cursor >= end) return MODULE_FRAME_INCOMPLETE;
    if (*cursor != protocol->field_separator) {
      if ((field_index + 1U != protocol->field_count) ||
          (protocol->alternate_payload_separator == 0U) ||
          (*cursor != protocol->alternate_payload_separator)) {
        return MODULE_FRAME_INVALID;
      }
    }
    ++cursor;
  }

  body_length = (size_t)fields[protocol->payload_length_field];
  if ((fields[protocol->payload_length_field] >
       protocol->maximum_payload_length) ||
      ((size_t)(end - cursor) < body_length)) {
    return (fields[protocol->payload_length_field] >
            protocol->maximum_payload_length)
               ? MODULE_FRAME_INVALID
               : MODULE_FRAME_INCOMPLETE;
  }
  *payload = cursor;
  *payload_length = body_length;
  cursor += body_length;
  if (protocol->parse_tail != NULL) {
    result = protocol->parse_tail(cursor, (size_t)(end - cursor),
                                  &tail_consumed);
    if (result != MODULE_FRAME_COMPLETE) return result;
  } else {
    if ((size_t)(end - cursor) < protocol->fixed_tail_length) {
      return MODULE_FRAME_INCOMPLETE;
    }
    if (memcmp(cursor, protocol->fixed_tail,
               protocol->fixed_tail_length) != 0) {
      return MODULE_FRAME_INVALID;
    }
    tail_consumed = protocol->fixed_tail_length;
  }
  if (tail_consumed > (size_t)(end - cursor)) return MODULE_FRAME_INVALID;
  *consumed = (size_t)(cursor - data) + tail_consumed;
  return MODULE_FRAME_COMPLETE;
}
