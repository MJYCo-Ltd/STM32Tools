#ifndef MQTT_LINE_COLLECTOR_H
#define MQTT_LINE_COLLECTOR_H

#include <AT/ModuleFrameParser.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef MQTT_LINE_COLLECTOR_CAPACITY
#define MQTT_LINE_COLLECTOR_CAPACITY 512U
#endif
#if MQTT_LINE_COLLECTOR_CAPACITY < 2U
#error "MQTT_LINE_COLLECTOR_CAPACITY must hold data and a terminator"
#endif

typedef void (*MqttLineCollectorCallback)(char *line, void *context);
typedef struct {
  char buffer[MQTT_LINE_COLLECTOR_CAPACITY];
  size_t length;
  MqttLineCollectorCallback callback;
  void *context;
  uint8_t discarding;
  size_t frame_remaining;
} MqttLineCollector;

static inline void MqttLineCollector_Dispatch(char *line, size_t length, void *context) {
  MqttLineCollector *collector = context;
  (void)length;
  if (collector->callback != NULL)
    collector->callback(line, collector->context);
}

static inline void MqttLineCollector_Init(MqttLineCollector *collector,
                                          MqttLineCollectorCallback callback, void *context) {
  if (collector == NULL)
    return;
  collector->length = 0U;
  collector->discarding = 0U;
  collector->frame_remaining = 0U;
  collector->buffer[0] = '\0';
  collector->callback = callback;
  collector->context = context;
}

/** Drop the rest of an interrupted record after a known transport loss. */
static inline void MqttLineCollector_Invalidate(MqttLineCollector *collector) {
  if (collector == NULL)
    return;
  collector->length = 0U;
  collector->buffer[0] = '\0';
  collector->discarding = 1U;
  collector->frame_remaining = 0U;
}

/** Task-context, single owner; callbacks must not reenter/reset this collector. */
static inline void MqttLineCollector_Feed(MqttLineCollector *collector, const uint8_t *data,
                                          uint16_t length) {
  ModuleLineCollector core;
  if (collector == NULL || data == NULL)
    return;
  core = (ModuleLineCollector){.buffer = collector->buffer,
                               .capacity = sizeof(collector->buffer),
                               .length = collector->length,
                               .discarding = collector->discarding,
                               .preserve_terminator = 1U,
                               .callback = MqttLineCollector_Dispatch,
                               .context = collector};
  ModuleFrameParser_FeedLines(&core, data, length);
  collector->length = core.length;
  collector->discarding = core.discarding;
}
typedef void (*MqttLengthFrameCallback)(char *frame, size_t length, void *context);
typedef void (*MqttTextCallback)(const uint8_t *text, size_t length, void *context);

/* Same storage and task owner as the legacy line collector. Protocol declarations
 * belong to the module model. Only fixed tails are supported here. Text bytes
 * go to the AT owner before the complete-line callback (early ACK ordering).
 * Frames, including rejected non-text bodies, NEVER enter the AT text path.
 * discarding: 0 prefix/header; 1 drain bad line; 2 drain oversized body;
 * 3 drain oversized tail; 4 ordinary text; 5 overflowed ordinary text;
 * 6 buffered body; 7 buffered tail. frame_remaining tracks accepted body/tail;
 * length remains the buffered frame size or oversized drain remainder.
 * No callback may reset/reenter this collector; borrowed memory expires on return.
 */
static inline void MqttLineCollector_FeedFrames(MqttLineCollector *collector,
    const uint8_t *data, uint16_t length, const ModuleLengthFrameProtocolEx *protocol,
    MqttLengthFrameCallback frame_callback, MqttTextCallback text_callback) {
  size_t i, prefix_length;
  if (collector == NULL || data == NULL || protocol == NULL ||
      protocol->base.prefix == NULL || protocol->base.fixed_tail == NULL ||
      !protocol->base.fixed_tail_length || protocol->base.parse_tail != NULL ||
      protocol->base.field_count > 8U) return;
  prefix_length = strlen(protocol->base.prefix);
  for (i = 0U; i < length; ++i) {
    uint8_t byte = data[i];
    if (collector->discarding == 1U || collector->discarding == 5U) {
      /* Ordinary AT responses (e.g. long scans) have their own larger buffer.
       * Losing this optional line copy must not truncate the AT byte stream. */
      if (collector->discarding == 5U && text_callback && byte != 0U)
        text_callback(&data[i], 1U, collector->context);
      if (byte == '\n') collector->discarding = 0U;
      continue;
    }
    if (collector->discarding == 2U || collector->discarding == 3U) {
      if (collector->discarding == 3U && byte != (uint8_t)protocol->base.fixed_tail[
          protocol->base.fixed_tail_length - collector->length]) {
        collector->length = 0U;
        collector->discarding = byte == '\n' ? 0U : 1U;
        collector->frame_remaining = 0U;
        continue;
      }
      if (--collector->length == 0U) {
        if (collector->discarding == 2U) {
          collector->discarding = 3U;
          collector->length = protocol->base.fixed_tail_length;
        } else collector->discarding = 0U;
      }
      continue;
    }
    if (collector->discarding == 6U) {
      collector->buffer[collector->length++] = (char)byte;
      collector->buffer[collector->length] = '\0';
      if (--collector->frame_remaining == 0U) {
        collector->discarding = 7U;
        collector->frame_remaining = protocol->base.fixed_tail_length;
      }
      continue;
    }
    if (collector->discarding == 7U) {
      size_t tail_index = protocol->base.fixed_tail_length - collector->frame_remaining;
      if (byte != (uint8_t)protocol->base.fixed_tail[tail_index]) {
        collector->length = 0U;
        collector->discarding = byte == '\n' ? 0U : 1U;
        collector->frame_remaining = 0U;
        continue;
      }
      collector->buffer[collector->length++] = (char)byte;
      collector->buffer[collector->length] = '\0';
      if (--collector->frame_remaining == 0U) {
        size_t completed = collector->length;
        collector->length = 0U;
        collector->discarding = 0U;
        if (frame_callback) frame_callback(collector->buffer, completed, collector->context);
      }
      continue;
    }
    if (collector->length >= sizeof(collector->buffer) - 1U ||
        (byte == 0U && collector->discarding == 4U)) {
      uint8_t text_overflow = collector->discarding == 4U && byte != 0U;
      if (text_overflow && text_callback) text_callback(&data[i], 1U, collector->context);
      collector->length = 0U;
      collector->discarding = byte == '\n' ? 0U : (text_overflow ? 5U : 1U);
      collector->frame_remaining = 0U;
      continue;
    }
    collector->buffer[collector->length++] = (char)byte;
    collector->buffer[collector->length] = '\0';
    if (collector->discarding == 0U) {
      uint32_t fields[8];
      const uint8_t *body;
      size_t body_length;
      ModuleFrameResult result;
      size_t compare = collector->length < prefix_length ? collector->length : prefix_length;
      if (memcmp(collector->buffer, protocol->base.prefix, compare)) {
        collector->discarding = 4U;
        if (text_callback) text_callback((const uint8_t *)collector->buffer,
                                         collector->length, collector->context);
      } else {
        result = ModuleFrameParser_ParseLengthHeader((const uint8_t *)collector->buffer,
            collector->length, protocol, fields, 8U, NULL, &body, &body_length);
        if (result == MODULE_FRAME_INVALID) {
          collector->length = 0U;
          collector->discarding = byte == '\n' ? 0U : 1U;
          collector->frame_remaining = 0U;
        } else if (result == MODULE_FRAME_COMPLETE) {
          size_t header_length = (size_t)(body - (const uint8_t *)collector->buffer);
          size_t available = collector->length - header_length;
          if (body_length > protocol->base.maximum_payload_length ||
              header_length + protocol->base.fixed_tail_length >= sizeof(collector->buffer) ||
              body_length >= sizeof(collector->buffer) - header_length - protocol->base.fixed_tail_length) {
            collector->length = body_length > available ? body_length - available : 0U;
            collector->discarding = collector->length ? 2U : 3U;
            collector->frame_remaining = 0U;
            if (!collector->length) collector->length = protocol->base.fixed_tail_length;
          } else {
            collector->discarding = body_length == 0U ? 7U : 6U;
            collector->frame_remaining = body_length == 0U ?
                protocol->base.fixed_tail_length : body_length;
          }
        }
        continue;
      }
    } else if (text_callback) text_callback(&data[i], 1U, collector->context);
    if (byte == '\n') {
      collector->length = 0U;
      collector->discarding = 0U;
      collector->frame_remaining = 0U;
      if (collector->callback) collector->callback(collector->buffer, collector->context);
    }
  }
}
#endif
