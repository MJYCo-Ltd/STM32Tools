#ifndef MQTT_LINE_COLLECTOR_H
#define MQTT_LINE_COLLECTOR_H

#include <stddef.h>
#include <stdint.h>

#ifndef MQTT_LINE_COLLECTOR_CAPACITY
#define MQTT_LINE_COLLECTOR_CAPACITY 512U
#endif

typedef void (*MqttLineCollectorCallback)(char *line, void *context);

typedef struct {
  char buffer[MQTT_LINE_COLLECTOR_CAPACITY];
  size_t length;
  MqttLineCollectorCallback callback;
  void *context;
} MqttLineCollector;

static inline void MqttLineCollector_Init(MqttLineCollector *collector,
                                          MqttLineCollectorCallback callback,
                                          void *context)
{
  if (collector == NULL) {
    return;
  }
  collector->length = 0U;
  collector->buffer[0] = '\0';
  collector->callback = callback;
  collector->context = context;
}

static inline void MqttLineCollector_Feed(MqttLineCollector *collector,
                                          const uint8_t *data,
                                          uint16_t length)
{
  uint16_t i;

  if ((collector == NULL) || (data == NULL)) {
    return;
  }
  for (i = 0U; i < length; ++i) {
    if (collector->length + 1U >= sizeof(collector->buffer)) {
      collector->length = 0U;
    }
    collector->buffer[collector->length++] = (char)data[i];
    if (data[i] == '\n') {
      collector->buffer[collector->length] = '\0';
      if (collector->callback != NULL) {
        collector->callback(collector->buffer, collector->context);
      }
      collector->length = 0U;
    }
  }
}

#endif
