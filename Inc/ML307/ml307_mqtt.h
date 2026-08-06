#ifndef STM32TOOLS_ML307_MQTT_H
#define STM32TOOLS_ML307_MQTT_H

#include <stddef.h>
#include <stdint.h>

#include <ML307/ml307.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ML307_MQTT_TOPIC_SIZE 128U
#define ML307_MQTT_PAYLOAD_SIZE 256U

typedef enum {
  ML307_MQTT_EVENT_NONE = 0,
  ML307_MQTT_EVENT_CONNECTION,
  ML307_MQTT_EVENT_PUBLISH,
  ML307_MQTT_EVENT_SUBACK,
  ML307_MQTT_EVENT_UNSUBACK,
  ML307_MQTT_EVENT_PUBACK,
  ML307_MQTT_EVENT_PUBREC,
  ML307_MQTT_EVENT_PUBCOMP,
  ML307_MQTT_EVENT_TIMEOUT,
  ML307_MQTT_EVENT_PINGRESP,
  ML307_MQTT_EVENT_PUBNMI,
  ML307_MQTT_EVENT_DROP
} ML307_MqttEventType;

typedef struct {
  ML307_MqttEventType type;
  uint8_t connect_id;
  int state;
  uint16_t message_id;
  uint8_t qos;
  uint32_t total_length;
  uint32_t payload_length;
  char topic[ML307_MQTT_TOPIC_SIZE];
  char payload[ML307_MQTT_PAYLOAD_SIZE];
} ML307_MqttEvent;

typedef ML307_Result (*ML307_MqttWriteCallback)(const uint8_t *data,
                                                 size_t length, void *context);

typedef struct {
  ML307_MqttWriteCallback write;
  void *context;
} ML307_MqttTransport;

ML307_Result ML307_MqttBuildCleanSession(char *output, size_t output_size,
                                          uint8_t connect_id,
                                          uint8_t clean_session);
ML307_Result ML307_MqttBuildConnect(char *output, size_t output_size,
                                    uint8_t connect_id, const char *host,
                                    uint16_t port, const char *client_id,
                                    const char *user, const char *password);
ML307_Result ML307_MqttBuildDisconnect(char *output, size_t output_size,
                                       uint8_t connect_id);
ML307_Result ML307_MqttBuildSubscribe(char *output, size_t output_size,
                                      uint8_t connect_id, const char *topic,
                                      uint8_t qos);
ML307_Result ML307_MqttBuildPublish(char *output, size_t output_size,
                                    uint8_t connect_id, const char *topic,
                                    uint8_t qos, uint8_t retain,
                                    const char *message);

ML307_Result ML307_MqttSendCleanSession(const ML307_MqttTransport *transport,
                                         uint8_t connect_id,
                                         uint8_t clean_session);
ML307_Result ML307_MqttSendConnect(const ML307_MqttTransport *transport,
                                   uint8_t connect_id, const char *host,
                                   uint16_t port, const char *client_id,
                                   const char *user, const char *password);
ML307_Result ML307_MqttSendDisconnect(const ML307_MqttTransport *transport,
                                      uint8_t connect_id);
ML307_Result ML307_MqttSendSubscribe(const ML307_MqttTransport *transport,
                                     uint8_t connect_id, const char *topic,
                                     uint8_t qos);
ML307_Result ML307_MqttSendPublish(const ML307_MqttTransport *transport,
                                   uint8_t connect_id, const char *topic,
                                   uint8_t qos, uint8_t retain,
                                   const char *message);

int ML307_MqttResponseHasError(const char *raw);
int ML307_MqttConnectResponseIsComplete(const char *raw, uint8_t connect_id);
ML307_Result ML307_MqttParseUrc(const char *raw, ML307_MqttEvent *event);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_ML307_MQTT_H */
