/*
 ******************************************************************************
 * @file           : ml307.c
 * @brief          : ML307 pack / unpack facade
 ******************************************************************************
 */
#include "ML307/ml307.h"

#include <stdlib.h>
#include <string.h>

#include "ML307/ml307_at.h"
#include "ML307/ml307_mqtt.h"
#include "ML307/ml307_parser.h"

static ML307_Cmd TypeToCmd(ML307_Type type)
{
  switch (type) {
  case ML307_TYPE_ATI:
    return ML307_CMD_ATI;
  case ML307_TYPE_CEREG:
    return ML307_CMD_CEREG;
  case ML307_TYPE_CIMI:
    return ML307_CMD_CIMI;
  case ML307_TYPE_CESQ:
    return ML307_CMD_CESQ;
  case ML307_TYPE_CGATT:
    return ML307_CMD_CGATT;
  default:
    return ML307_CMD_COUNT;
  }
}

static int IsAtQuery(ML307_Type type)
{
  return TypeToCmd(type) != ML307_CMD_COUNT;
}

static int ParseCesqRsrp(const char *info)
{
  long fields[6];
  uint8_t count = 0U;
  const char *cursor = info;
  char *end;

  if (info == NULL) {
    return -1;
  }
  while ((count < 6U) && (*cursor != '\0')) {
    fields[count] = strtol(cursor, &end, 10);
    if (end == cursor) {
      return -1;
    }
    ++count;
    cursor = end;
    if (*cursor == ',') {
      ++cursor;
    } else if ((*cursor != '\0') && (count < 6U)) {
      return -1;
    }
  }
  return (count < 6U) ? -1 : (int)fields[5];
}

static ML307_Result FinishPack(char *packet, size_t packet_size, size_t *length,
                               ML307_Result result)
{
  if (result != ML307_RESULT_OK) {
    if ((packet != NULL) && (packet_size > 0U)) {
      packet[0] = '\0';
    }
    if (length != NULL) {
      *length = 0U;
    }
    return result;
  }
  if (length != NULL) {
    *length = strlen(packet);
  }
  return ML307_RESULT_OK;
}

const char *ML307_TypeName(ML307_Type type)
{
  switch (type) {
  case ML307_TYPE_ATI:
    return "ATI";
  case ML307_TYPE_CEREG:
    return "CEREG";
  case ML307_TYPE_CIMI:
    return "CIMI";
  case ML307_TYPE_CESQ:
    return "CESQ";
  case ML307_TYPE_CGATT:
    return "CGATT";
  case ML307_TYPE_SLEEP:
    return "SLEEP";
  case ML307_TYPE_MQTT_DISCONNECT:
    return "MQTT_DISC";
  case ML307_TYPE_MQTT_CLEAN:
    return "MQTT_CLEAN";
  case ML307_TYPE_MQTT_CONNECT:
    return "MQTT_CONN";
  case ML307_TYPE_MQTT_SUBSCRIBE:
    return "MQTT_SUB";
  case ML307_TYPE_MQTT_PUBLISH:
    return "MQTT_PUB";
  default:
    return "?";
  }
}

ML307_Result ML307_Pack(const ML307_Content *content, char *packet,
                        size_t packet_size, size_t *length)
{
  ML307_Result result;
  const char *topic;

  if ((content == NULL) || (packet == NULL) || (packet_size == 0U)) {
    return FinishPack(packet, packet_size, length,
                      ML307_RESULT_INVALID_ARGUMENT);
  }

  if (IsAtQuery(content->type)) {
    result = ML307_BuildQueryCommand(packet, packet_size,
                                     TypeToCmd(content->type), NULL);
    return FinishPack(packet, packet_size, length, result);
  }

  topic = (content->topic != NULL) ? content->topic : content->host;

  switch (content->type) {
  case ML307_TYPE_SLEEP:
    result = ML307_BuildSleep(packet, packet_size);
    break;
  case ML307_TYPE_MQTT_DISCONNECT:
    result = ML307_MqttBuildDisconnect(packet, packet_size, content->id);
    break;
  case ML307_TYPE_MQTT_CLEAN:
    result = ML307_MqttBuildCleanSession(packet, packet_size, content->id,
                                         content->flag);
    break;
  case ML307_TYPE_MQTT_CONNECT:
    result = ML307_MqttBuildConnect(
        packet, packet_size, content->id, content->host, content->port,
        (content->client_id != NULL) ? content->client_id : "",
        (content->user != NULL) ? content->user : "",
        (content->password != NULL) ? content->password : "");
    break;
  case ML307_TYPE_MQTT_SUBSCRIBE:
    result = ML307_MqttBuildSubscribe(packet, packet_size, content->id, topic,
                                      content->qos);
    break;
  case ML307_TYPE_MQTT_PUBLISH:
    result = ML307_MqttBuildPublish(
        packet, packet_size, content->id, topic, content->qos, content->retain,
        (content->message != NULL) ? content->message : "");
    break;
  default:
    result = ML307_RESULT_INVALID_VALUE;
    break;
  }
  return FinishPack(packet, packet_size, length, result);
}

int ML307_IsComplete(const char *packet, ML307_Type expect, uint8_t id)
{
  if (packet == NULL) {
    return 0;
  }
  if (expect == ML307_TYPE_MQTT_CONNECT) {
    return ML307_MqttConnectResponseIsComplete(packet, id);
  }
  return ML307_ResponseIsComplete(packet);
}

ML307_Result ML307_Unpack(const char *packet, ML307_Type expect,
                          ML307_Data *out)
{
  ML307_ParsedResponse parsed;
  ML307_MqttEvent mqtt;
  ML307_ParseResult parse_result;
  const char *expected_type;
  ML307_Cmd cmd;

  if ((packet == NULL) || (out == NULL)) {
    return ML307_RESULT_INVALID_ARGUMENT;
  }
  memset(out, 0, sizeof(*out));
  out->type = expect;
  out->value = -1;

  if ((expect == ML307_TYPE_MQTT_CONNECT) ||
      (expect == ML307_TYPE_MQTT_SUBSCRIBE) ||
      (expect == ML307_TYPE_MQTT_PUBLISH) ||
      (expect == ML307_TYPE_MQTT_CLEAN) ||
      (expect == ML307_TYPE_MQTT_DISCONNECT)) {
    if (ML307_MqttResponseHasError(packet) != 0) {
      out->error = 1U;
      (void)strncpy(out->name, "ERROR", sizeof(out->name) - 1U);
      (void)strncpy(out->text, "Module rejected command",
                    sizeof(out->text) - 1U);
      return ML307_RESULT_ERROR_RESPONSE;
    }

    if (expect == ML307_TYPE_MQTT_CONNECT) {
      if (ML307_MqttParseUrc(packet, &mqtt) != ML307_RESULT_OK) {
        return ML307_RESULT_NOT_FOUND;
      }
      out->id = mqtt.connect_id;
      out->value = mqtt.state;
      out->message_id = mqtt.message_id;
      out->qos = mqtt.qos;
      (void)strncpy(out->name, "MQTT", sizeof(out->name) - 1U);
      (void)strncpy(out->topic, mqtt.topic, sizeof(out->topic) - 1U);
      (void)strncpy(out->payload, mqtt.payload, sizeof(out->payload) - 1U);
      if (mqtt.type == ML307_MQTT_EVENT_CONNECTION) {
        out->ok = (mqtt.state == 0) ? 1U : 0U;
        out->error = (mqtt.state == 0) ? 0U : 1U;
        return (mqtt.state == 0) ? ML307_RESULT_OK : ML307_RESULT_ERROR_RESPONSE;
      }
      out->ok = 1U;
      return ML307_RESULT_OK;
    }

    out->ok = 1U;
    (void)strncpy(out->name, ML307_TypeName(expect), sizeof(out->name) - 1U);
    (void)strncpy(out->text, "OK", sizeof(out->text) - 1U);
    if (ML307_MqttParseUrc(packet, &mqtt) == ML307_RESULT_OK) {
      out->id = mqtt.connect_id;
      out->value = mqtt.state;
      out->message_id = mqtt.message_id;
      out->qos = mqtt.qos;
      (void)strncpy(out->topic, mqtt.topic, sizeof(out->topic) - 1U);
      (void)strncpy(out->payload, mqtt.payload, sizeof(out->payload) - 1U);
    }
    return ML307_RESULT_OK;
  }

  cmd = TypeToCmd(expect);
  if (expect == ML307_TYPE_SLEEP) {
    if (ML307_ResponseIsComplete(packet) == 0) {
      return ML307_RESULT_NOT_FOUND;
    }
    if ((strstr(packet, "ERROR") != NULL) ||
        (strstr(packet, "+CME ERROR:") != NULL)) {
      out->error = 1U;
      (void)strncpy(out->name, "ERROR", sizeof(out->name) - 1U);
      return ML307_RESULT_ERROR_RESPONSE;
    }
    out->ok = 1U;
    (void)strncpy(out->name, "SLEEP", sizeof(out->name) - 1U);
    (void)strncpy(out->text, "OK", sizeof(out->text) - 1U);
    return ML307_RESULT_OK;
  }

  if (cmd == ML307_CMD_COUNT) {
    return ML307_RESULT_INVALID_VALUE;
  }
  expected_type = ML307_GetExpectedResponseType(cmd);
  if (expected_type == NULL) {
    return ML307_RESULT_INVALID_VALUE;
  }

  parse_result = ML307_ParseResponse(packet, expected_type, &parsed);
  out->ok = parsed.has_ok;
  out->error = parsed.has_error;
  (void)strncpy(out->name, parsed.type, sizeof(out->name) - 1U);
  (void)strncpy(out->text, parsed.info, sizeof(out->text) - 1U);

  if (parse_result == ML307_PARSE_ERROR_RESPONSE) {
    return ML307_RESULT_ERROR_RESPONSE;
  }
  if (parse_result == ML307_PARSE_TRUNCATED) {
    return ML307_RESULT_BUFFER_TOO_SMALL;
  }
  if (parse_result != ML307_PARSE_OK) {
    return ML307_RESULT_NOT_FOUND;
  }

  if (expect == ML307_TYPE_CESQ) {
    out->value = ParseCesqRsrp(out->text);
  }
  return ML307_RESULT_OK;
}
