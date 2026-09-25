/*
 ******************************************************************************
 * @file           : ml307_mqtt.h
 * @brief          : ML307 MQTT command helpers and typed control responses
 *
 * Use ML307_Pack / ML307_Unpack for requests. Adapters may consume the bounded
 * control/command response parsers here instead of re-parsing MQTT text.
 ******************************************************************************
 */
#ifndef STM32TOOLS_ML307_MQTT_H
#define STM32TOOLS_ML307_MQTT_H

#include <stddef.h>
#include <stdint.h>

#include <ML307/ml307.h>

#ifdef __cplusplus
extern "C" {
#endif

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
  char topic[ML307_TOPIC_SIZE];
  char payload[ML307_PAYLOAD_SIZE];
} ML307_MqttEvent;

ML307_Result ML307_MqttBuildCleanSession(char *output, size_t output_size,
                                         uint8_t connect_id,
                                         uint8_t clean_session);
ML307_Result ML307_MqttBuildSslConfig(char *output, size_t output_size,
                                      uint8_t connect_id, uint8_t ssl_enable,
                                      uint8_t ssl_id);
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

/** One complete text PUBLISH slice; exact body length is required. Embedded
 * NUL/CR/LF are rejected because the product consumes C strings. A valid partial
 * payload is reported as part<total and must NOT be delivered as a full message.
 * Does not reassemble fragments or claim support for arbitrary binary payloads.
 */
ML307_Result ML307_MqttParseTextPublish(const uint8_t *line, size_t length,
                                        ML307_MqttEvent *event);

int ML307_MqttResponseHasError(const char *raw);
int ML307_MqttConnectResponseIsComplete(const char *raw, uint8_t connect_id);
/** Parse one complete text PUBLISH line (optional terminal CRLF).
 * Payload length is checked against the bytes actually present. Embedded NUL,
 * CR/LF, binary payloads and fragment reassembly are not supported by this API.
 * Output is cleared on failure; total >= fragment, CID 0..5, MID <= 65535.
 */
ML307_Result ML307_MqttParsePublishUrc(const uint8_t *line, size_t length,
                                      ML307_MqttEvent *event);

ML307_Result ML307_MqttParseUrc(const char *raw, ML307_MqttEvent *event);

/**
 * Parse ONE complete control line: conn, suback, puback or timeout. The caller
 * establishes the line boundary (e.g. a collector callback); a terminal LF is
 * optional here. Length excludes any C-string terminator. No NUL is required.
 * Exact field counts, unsigned ranges and status codes are checked. Unsupported
 * URC kinds return NOT_FOUND. A non-NULL event is cleared on every failure.
 * This API is not a length-delimited/binary MQTT PUBLISH parser.
 */
ML307_Result ML307_MqttParseControlUrc(const uint8_t *line, size_t length,
                                      ML307_MqttEvent *event);

/**
 * Scan a bounded command response; only LF-terminated lines are eligible.
 * The first matching command line must have the exact field count. Outputs
 * change only on success. These parse metadata, NOT command/broker success:
 * the adapter must still check the AT final result and match id/mid to its
 * outstanding command. Plain OK never substitutes for a broker ACK.
 */
ML307_Result ML307_MqttParseSubResponse(const uint8_t *response, size_t length,
                                       uint8_t *connect_id, uint16_t *mid);
ML307_Result ML307_MqttParsePubResponse(const uint8_t *response, size_t length,
                                       uint8_t *connect_id, uint16_t *mid);
ML307_Result ML307_MqttParseStateResponse(const uint8_t *response, size_t length,
                                         uint32_t *state);
/** Last valid, LF-terminated conn line for the requested connection id. */
ML307_Result ML307_MqttParseConnectionResponse(const uint8_t *response,
                                              size_t length, uint8_t connect_id,
                                              uint32_t *state);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_ML307_MQTT_H */
