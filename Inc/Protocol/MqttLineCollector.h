#ifndef MQTT_LINE_COLLECTOR_H
#define MQTT_LINE_COLLECTOR_H

#include <AT/ModuleFrameParser.h>
#include <stddef.h>
#include <stdint.h>

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
#endif
