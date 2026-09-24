#ifndef STM32TOOLS_NET_COMM_ADAPTER_H
#define STM32TOOLS_NET_COMM_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

#include <Net/ConnectTransport.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t CommAdapterId;

#define COMM_ADAPTER_ID_NONE 0U
#define COMM_ADAPTER_REGISTRY_MAX_BITMASK 8U

typedef struct {
  CommAdapterId id;
  const char *name;

  /* Product/session tracker channel used to correlate asynchronous MQTT
   * commands. Zero means this adapter does not expose managed MQTT commands. */
  uint8_t mqtt_command_channel;
  /* Lower value is preferred for bulk/download routing. */
  uint8_t download_priority;

  size_t topic_capacity;
  size_t payload_capacity;

  uint8_t (*init)(void);
  uint8_t (*is_selectable)(void);
  uint8_t (*is_online)(void);
  uint8_t (*is_busy)(void);
  uint8_t (*is_mqtt_connected)(void);
  void (*set_selected)(uint8_t selected);
  int (*get_signal_dbm)(void);
  void (*format_diagnostics)(char *out, size_t capacity);

  uint8_t (*mqtt_connect)(void);
  uint8_t (*mqtt_publish)(const char *topic, const char *payload, uint8_t qos,
                          uint8_t retain);
  uint8_t (*mqtt_subscribe)(const char *topic, uint8_t qos);

  void (*tick)(void);
  uint8_t (*recovery_required)(void);
  uint8_t (*recover)(void);

  uint8_t (*download_ownership_available)(void);
  uint8_t (*download_start)(const ConnectDownloadRequest *request);
  uint8_t (*download_get_event)(ConnectDownloadEvent *event);
  uint8_t (*download_acknowledge)(uint8_t accepted);
} CommAdapterOps;

typedef struct {
  const CommAdapterOps *items;
  size_t count;
} CommAdapterRegistry;

static inline size_t CommAdapterRegistry_Count(const CommAdapterRegistry *registry)
{
  return (registry != NULL) ? registry->count : 0U;
}

static inline const CommAdapterOps *CommAdapterRegistry_At(
    const CommAdapterRegistry *registry, size_t index)
{
  return ((registry != NULL) && (registry->items != NULL) &&
          (index < registry->count))
             ? &registry->items[index]
             : NULL;
}

static inline const CommAdapterOps *CommAdapterRegistry_Find(
    const CommAdapterRegistry *registry, CommAdapterId id)
{
  size_t i;
  if ((registry == NULL) || (registry->items == NULL) ||
      (id == COMM_ADAPTER_ID_NONE)) {
    return NULL;
  }
  for (i = 0U; i < registry->count; ++i) {
    if (registry->items[i].id == id) return &registry->items[i];
  }
  return NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_NET_COMM_ADAPTER_H */
