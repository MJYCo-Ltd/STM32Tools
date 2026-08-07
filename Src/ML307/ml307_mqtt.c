#include <ML307/ml307_mqtt.h>

#include <AT/at_codec.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ML307_Result MqttFormat(char *output, size_t output_size,
                               const char *format, ...)
{
  AT_CodecResult result;
  va_list args;

  va_start(args, format);
  result = AT_FormatV(output, output_size, format, args);
  va_end(args);

  if (result == AT_CODEC_OK) {
    return ML307_RESULT_OK;
  }
  if (result == AT_CODEC_BUFFER_TOO_SMALL) {
    return ML307_RESULT_BUFFER_TOO_SMALL;
  }
  return (result == AT_CODEC_FORMAT_ERROR) ? ML307_RESULT_INVALID_VALUE
                                           : ML307_RESULT_INVALID_ARGUMENT;
}

static int MqttStringIsValid(const char *value, size_t max_length)
{
  size_t length;

  if (value == NULL) {
    return 0;
  }
  length = strlen(value);
  return (length <= max_length) && (strchr(value, '"') == NULL) &&
         (strchr(value, '\r') == NULL) && (strchr(value, '\n') == NULL);
}

static int MqttParseInt(const char **cursor, long *value)
{
  char *end;

  *value = strtol(*cursor, &end, 10);
  if (end == *cursor) {
    return 0;
  }
  *cursor = end;
  return 1;
}

ML307_Result ML307_MqttBuildCleanSession(char *output, size_t output_size,
                                          uint8_t connect_id,
                                          uint8_t clean_session)
{
  if ((connect_id > 5U) || (clean_session > 1U)) {
    return ML307_RESULT_INVALID_VALUE;
  }
  return MqttFormat(output, output_size, "AT+MQTTCFG=\"clean\",%u,%u\r\n",
                    (unsigned int)connect_id, (unsigned int)clean_session);
}

ML307_Result ML307_MqttBuildConnect(char *output, size_t output_size,
                                    uint8_t connect_id, const char *host,
                                    uint16_t port, const char *client_id,
                                    const char *user, const char *password)
{
  if ((connect_id > 5U) || (port == 0U) ||
      !MqttStringIsValid(host, 128U) || (host[0] == '\0') ||
      !MqttStringIsValid(client_id, 128U) ||
      !MqttStringIsValid(user, 128U) ||
      !MqttStringIsValid(password, 256U)) {
    return ML307_RESULT_INVALID_VALUE;
  }
  return MqttFormat(output, output_size,
                    "AT+MQTTCONN=%u,\"%s\",%u,\"%s\",\"%s\",\"%s\"\r\n",
                    (unsigned int)connect_id, host, (unsigned int)port,
                    client_id, user, password);
}

ML307_Result ML307_MqttBuildDisconnect(char *output, size_t output_size,
                                       uint8_t connect_id)
{
  if (connect_id > 5U) {
    return ML307_RESULT_INVALID_VALUE;
  }
  return MqttFormat(output, output_size, "AT+MQTTDISC=%u\r\n",
                    (unsigned int)connect_id);
}

ML307_Result ML307_MqttBuildSubscribe(char *output, size_t output_size,
                                      uint8_t connect_id, const char *topic,
                                      uint8_t qos)
{
  if ((connect_id > 5U) || (qos > 2U) ||
      !MqttStringIsValid(topic, 256U) || (topic[0] == '\0')) {
    return ML307_RESULT_INVALID_VALUE;
  }
  return MqttFormat(output, output_size, "AT+MQTTSUB=%u,\"%s\",%u\r\n",
                    (unsigned int)connect_id, topic, (unsigned int)qos);
}

ML307_Result ML307_MqttBuildPublish(char *output, size_t output_size,
                                    uint8_t connect_id, const char *topic,
                                    uint8_t qos, uint8_t retain,
                                    const char *message)
{
  size_t message_length;

  if ((connect_id > 5U) || (qos > 2U) || (retain > 1U) ||
      !MqttStringIsValid(topic, 256U) || (topic[0] == '\0') ||
      !MqttStringIsValid(message, ML307_PAYLOAD_SIZE - 1U)) {
    return ML307_RESULT_INVALID_VALUE;
  }
  message_length = strlen(message);
  return MqttFormat(
      output, output_size, "AT+MQTTPUB=%u,\"%s\",%u,%u,0,%u,\"%s\"\r\n",
      (unsigned int)connect_id, topic, (unsigned int)qos,
      (unsigned int)retain, (unsigned int)message_length, message);
}

int ML307_MqttResponseHasError(const char *raw)
{
  if (raw == NULL) {
    return 0;
  }
  return (strstr(raw, "+CME ERROR:") != NULL) ||
         (strstr(raw, "\r\nERROR\r\n") != NULL);
}

int ML307_MqttConnectResponseIsComplete(const char *raw, uint8_t connect_id)
{
  ML307_MqttEvent event;

  if (ML307_MqttResponseHasError(raw)) {
    return 1;
  }
  return (ML307_MqttParseUrc(raw, &event) == ML307_RESULT_OK) &&
         (event.type == ML307_MQTT_EVENT_CONNECTION) &&
         (event.connect_id == connect_id) && (event.state != 1);
}

ML307_Result ML307_MqttParseUrc(const char *raw, ML307_MqttEvent *event)
{
  const char *urc;
  char name[16];
  int connect_id;
  int value1 = 0;
  int value2 = 0;

  if ((raw == NULL) || (event == NULL)) {
    return ML307_RESULT_INVALID_ARGUMENT;
  }
  memset(event, 0, sizeof(*event));
  urc = strstr(raw, "+MQTTURC: \"");
  if ((urc == NULL) ||
      (sscanf(urc, "+MQTTURC: \"%15[^\"]\",%d", name, &connect_id) != 2) ||
      (connect_id < 0) || (connect_id > 5)) {
    return ML307_RESULT_NOT_FOUND;
  }
  event->connect_id = (uint8_t)connect_id;

  if (strcmp(name, "conn") == 0) {
    if (sscanf(urc, "+MQTTURC: \"conn\",%d,%d", &connect_id, &value1) != 2) {
      return ML307_RESULT_INVALID_VALUE;
    }
    event->type = ML307_MQTT_EVENT_CONNECTION;
    event->state = value1;
    return ML307_RESULT_OK;
  }

  if (strcmp(name, "publish") == 0) {
    const char *cursor = strchr(urc, ',');
    const char *topic_end;
    long parsed;
    size_t length;

    if (cursor == NULL) {
      return ML307_RESULT_INVALID_VALUE;
    }
    ++cursor;
    if (!MqttParseInt(&cursor, &parsed) || (*cursor++ != ',')) {
      return ML307_RESULT_INVALID_VALUE;
    }
    event->connect_id = (uint8_t)parsed;
    if (!MqttParseInt(&cursor, &parsed) || (*cursor++ != ',') ||
        (*cursor++ != '"')) {
      return ML307_RESULT_INVALID_VALUE;
    }
    event->message_id = (uint16_t)parsed;
    topic_end = strchr(cursor, '"');
    if (topic_end == NULL) {
      return ML307_RESULT_INVALID_VALUE;
    }
    length = (size_t)(topic_end - cursor);
    if (length >= sizeof(event->topic)) {
      return ML307_RESULT_BUFFER_TOO_SMALL;
    }
    memcpy(event->topic, cursor, length);
    event->topic[length] = '\0';
    cursor = topic_end + 1;
    if ((*cursor++ != ',') || !MqttParseInt(&cursor, &parsed)) {
      return ML307_RESULT_INVALID_VALUE;
    }
    event->total_length = (uint32_t)parsed;
    if ((*cursor++ != ',') || !MqttParseInt(&cursor, &parsed) ||
        (*cursor++ != ',')) {
      return ML307_RESULT_INVALID_VALUE;
    }
    event->payload_length = (uint32_t)parsed;
    length = strcspn(cursor, "\r\n");
    if (length >= sizeof(event->payload)) {
      return ML307_RESULT_BUFFER_TOO_SMALL;
    }
    memcpy(event->payload, cursor, length);
    event->payload[length] = '\0';
    event->type = ML307_MQTT_EVENT_PUBLISH;
    return ML307_RESULT_OK;
  }

  if (sscanf(urc, "+MQTTURC: \"%15[^\"]\",%d,%d,%d", name, &connect_id,
             &value1, &value2) < 3) {
    return ML307_RESULT_INVALID_VALUE;
  }
  event->message_id = (uint16_t)value1;
  event->state = value2;
  if (strcmp(name, "suback") == 0) {
    event->type = ML307_MQTT_EVENT_SUBACK;
    event->qos = (uint8_t)value2;
  } else if (strcmp(name, "unsuback") == 0) {
    event->type = ML307_MQTT_EVENT_UNSUBACK;
  } else if (strcmp(name, "puback") == 0) {
    event->type = ML307_MQTT_EVENT_PUBACK;
  } else if (strcmp(name, "pubrec") == 0) {
    event->type = ML307_MQTT_EVENT_PUBREC;
  } else if (strcmp(name, "pubcomp") == 0) {
    event->type = ML307_MQTT_EVENT_PUBCOMP;
  } else if (strcmp(name, "timeout") == 0) {
    event->type = ML307_MQTT_EVENT_TIMEOUT;
  } else if (strcmp(name, "pingresp") == 0) {
    event->type = ML307_MQTT_EVENT_PINGRESP;
  } else if (strcmp(name, "pubnmi") == 0) {
    event->type = ML307_MQTT_EVENT_PUBNMI;
  } else if (strcmp(name, "drop") == 0) {
    event->type = ML307_MQTT_EVENT_DROP;
  } else {
    return ML307_RESULT_NOT_FOUND;
  }
  return ML307_RESULT_OK;
}
