#ifndef MQTT_LINE_COLLECTOR_H
#define MQTT_LINE_COLLECTOR_H

#include <AT/ModuleFrameParser.h>
#ifndef MQTT_LINE_COLLECTOR_CAPACITY
#define MQTT_LINE_COLLECTOR_CAPACITY 512U
#endif
#if MQTT_LINE_COLLECTOR_CAPACITY < 2
#error "MQTT line capacity must allow data and a terminator"
#endif

typedef void (*MqttLineCollectorCallback)(char *line, void *context);
typedef struct {
  char buffer[MQTT_LINE_COLLECTOR_CAPACITY];
  ModuleLineCollector core;
  MqttLineCollectorCallback callback;
  void *context;
} MqttLineCollector;

static inline void MqttLineCollector_Dispatch(char *line, size_t length, void *context)
{
  MqttLineCollector *collector = context;
  (void)length;
  if (collector->callback) collector->callback(line, collector->context);
}

/* Keep the public callback's original CR/LF convention. Lifetime/serialization
 * belong to the caller. An overflowing line is discarded, never truncated. */
static inline void MqttLineCollector_Init(MqttLineCollector *collector,
                                          MqttLineCollectorCallback callback,
                                          void *context)
{
  if (!collector) return;
  collector->callback = callback;
  collector->context = context;
  ModuleFrameParser_InitLineCollector(&collector->core, collector->buffer,
      sizeof(collector->buffer), MqttLineCollector_Dispatch, collector);
}

static inline void MqttLineCollector_Feed(MqttLineCollector *collector,
                                          const uint8_t *data, uint16_t length)
{
  if (collector) ModuleFrameParser_FeedRawLines(&collector->core, data, length);
}
#endif
