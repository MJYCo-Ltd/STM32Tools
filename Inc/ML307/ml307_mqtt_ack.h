#ifndef STM32TOOLS_ML307_MQTT_ACK_H
#define STM32TOOLS_ML307_MQTT_ACK_H

#include <ML307/ml307_mqtt.h>

typedef enum {
  ML307_MQTT_ACK_IGNORE = 0,
  ML307_MQTT_ACK_SUCCESS = 1,
  ML307_MQTT_ACK_FAILURE = 2
} ML307_MqttAckResult;

/* Typed ML307 broker acknowledgement semantics, not a product DB receipt.
 * Caller owns outstanding command/session lifetime and passes its id/mid.
 * No parsing here: the control-line parser remains the single grammar owner. */
static inline ML307_MqttAckResult ML307_MqttClassifyAck(
    const ML307_MqttEvent *event, uint8_t expect_subscribe,
    uint8_t connect_id, uint16_t mid)
{
  if (event == NULL || connect_id > 5U || event->connect_id != connect_id ||
      event->message_id != mid) return ML307_MQTT_ACK_IGNORE;
  if (event->type == ML307_MQTT_EVENT_TIMEOUT) return ML307_MQTT_ACK_FAILURE;
  if (expect_subscribe != 0U && event->type == ML307_MQTT_EVENT_SUBACK) {
    if (event->state == 128) return ML307_MQTT_ACK_FAILURE;
    if (event->state >= 0 && event->state <= 2) return ML307_MQTT_ACK_SUCCESS;
  } else if (expect_subscribe == 0U && event->type == ML307_MQTT_EVENT_PUBACK) {
    if (event->state >= 0 && event->state <= 1) return ML307_MQTT_ACK_SUCCESS;
  }
  return ML307_MQTT_ACK_IGNORE;
}
#endif
