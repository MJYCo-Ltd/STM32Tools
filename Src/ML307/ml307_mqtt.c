#include <ML307/ml307_mqtt.h>

#include <AT/at_codec.h>
#include <AT/ModuleFrameParser.h>

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


/* One strict grammar shared by the public response APIs and legacy URC facade.
 * This is protocol decoding only: outstanding-command correlation stays in the
 * product adapter. ParseUnsigned supplies bounded conversion/overflow checks.
 */
typedef struct {
  ML307_MqttEventType type;
  uint32_t fields[3];
} MqttControl;

static ML307_Result MqttParseFields(AT_Line line, const char *prefix,
                                    uint32_t *fields, size_t count)
{
  size_t i;
  /* NUL/control delimiters inside a numeric record must not hide a suffix. */
  if (memchr(line.data, '\0', line.length) != NULL) {
    return ML307_RESULT_INVALID_VALUE;
  }
  AT_TrimLine(&line);
  if (!AT_LineStartsWith(&line, prefix)) return ML307_RESULT_NOT_FOUND;
  line.data += strlen(prefix);
  line.length -= strlen(prefix);
  if (memchr(line.data, '\r', line.length) != NULL ||
      memchr(line.data, '\n', line.length) != NULL) {
    return ML307_RESULT_INVALID_VALUE;
  }
  for (i = 0U; i < count; ++i) {
    const uint8_t *cursor;
    const uint8_t *end;
    AT_TrimLine(&line);
    if (i != 0U) {
      if (!AT_LineStartsWith(&line, ",")) return ML307_RESULT_INVALID_VALUE;
      ++line.data;
      --line.length;
      AT_TrimLine(&line);
    }
    cursor = (const uint8_t *)line.data;
    end = cursor + line.length;
    if (ModuleFrameParser_ParseUnsigned(&cursor, end, UINT32_MAX, &fields[i]) !=
        MODULE_FRAME_COMPLETE) return ML307_RESULT_INVALID_VALUE;
    line.data = (const char *)cursor;
    line.length = (size_t)(end - cursor);
  }
  AT_TrimLine(&line);
  return (line.length == 0U) ? ML307_RESULT_OK : ML307_RESULT_INVALID_VALUE;
}

static ML307_Result MqttParseControl(AT_Line line, MqttControl *control)
{
  static const struct {
    const char *name_prefix;
    const char *fields_prefix;
    ML307_MqttEventType type;
    size_t count;
  } schemas[] = {
      {"+MQTTURC: \"conn\"", "+MQTTURC: \"conn\",", ML307_MQTT_EVENT_CONNECTION, 2U},
      {"+MQTTURC: \"suback\"", "+MQTTURC: \"suback\",", ML307_MQTT_EVENT_SUBACK, 3U},
      {"+MQTTURC: \"puback\"", "+MQTTURC: \"puback\",", ML307_MQTT_EVENT_PUBACK, 3U},
      {"+MQTTURC: \"timeout\"", "+MQTTURC: \"timeout\",", ML307_MQTT_EVENT_TIMEOUT, 2U}};
  size_t i;
  AT_TrimLine(&line);
  for (i = 0U; i < sizeof(schemas) / sizeof(schemas[0]); ++i) {
    uint32_t fields[3] = {0U};
    if (!AT_LineStartsWith(&line, schemas[i].name_prefix)) continue;
    if (MqttParseFields(line, schemas[i].fields_prefix, fields, schemas[i].count) !=
        ML307_RESULT_OK || fields[0] > 5U) return ML307_RESULT_INVALID_VALUE;
    if (schemas[i].type == ML307_MQTT_EVENT_CONNECTION) {
      if (fields[1] > 6U && fields[1] != 255U) return ML307_RESULT_INVALID_VALUE;
    } else {
      if (fields[1] > UINT16_MAX) return ML307_RESULT_INVALID_VALUE;
      if (schemas[i].type == ML307_MQTT_EVENT_SUBACK &&
          fields[2] > 2U && fields[2] != 128U) return ML307_RESULT_INVALID_VALUE;
      if (schemas[i].type == ML307_MQTT_EVENT_PUBACK && fields[2] > 1U)
        return ML307_RESULT_INVALID_VALUE;
    }
    control->type = schemas[i].type;
    memcpy(control->fields, fields, sizeof(fields));
    return ML307_RESULT_OK;
  }
  return ML307_RESULT_NOT_FOUND;
}

ML307_Result ML307_MqttParseControlUrc(const uint8_t *line, size_t length,
                                      ML307_MqttEvent *event)
{
  MqttControl control;
  ML307_Result result;
  if (event == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  memset(event, 0, sizeof(*event));
  if (line == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  if (memchr(line, '\0', length) != NULL) return ML307_RESULT_INVALID_VALUE;
  result = MqttParseControl((AT_Line){(const char *)line, length}, &control);
  if (result != ML307_RESULT_OK) return result;
  event->type = control.type;
  event->connect_id = (uint8_t)control.fields[0];
  if (control.type == ML307_MQTT_EVENT_CONNECTION) {
    event->state = (int)control.fields[1];
  } else {
    event->message_id = (uint16_t)control.fields[1];
    event->state = (int)control.fields[2];
    if (control.type == ML307_MQTT_EVENT_SUBACK) {
      event->qos = (uint8_t)control.fields[2];
    }
  }
  return ML307_RESULT_OK;
}

static ML307_Result MqttParseResponseFields(const uint8_t *response, size_t length,
                                             const char *prefix, uint32_t *fields,
                                             size_t count)
{
  const uint8_t *data;
  size_t line_length;
  size_t offset = 0U;
  if (response == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  while (ModuleFrameParser_NextLine(response, length, &offset, &data, &line_length)) {
    AT_Line line = {(const char *)data, line_length};
    AT_TrimLine(&line);
    if (AT_LineStartsWith(&line, prefix)) {
      return MqttParseFields((AT_Line){(const char *)data, line_length},
                             prefix, fields, count);
    }
  }
  return ML307_RESULT_NOT_FOUND;
}

static ML307_Result MqttParseCommandAck(const uint8_t *response, size_t length,
                                        const char *prefix, size_t count,
                                        uint8_t *connect_id, uint16_t *mid)
{
  uint32_t fields[3];
  ML307_Result result;
  if (connect_id == NULL || mid == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  result = MqttParseResponseFields(response, length, prefix, fields, count);
  if (result != ML307_RESULT_OK) return result;
  if (fields[0] > 5U || fields[1] > UINT16_MAX) return ML307_RESULT_INVALID_VALUE;
  *connect_id = (uint8_t)fields[0];
  *mid = (uint16_t)fields[1];
  return ML307_RESULT_OK;
}

ML307_Result ML307_MqttParseSubResponse(const uint8_t *response, size_t length,
                                       uint8_t *connect_id, uint16_t *mid)
{
  return MqttParseCommandAck(response, length, "+MQTTSUB:", 2U, connect_id, mid);
}

ML307_Result ML307_MqttParsePubResponse(const uint8_t *response, size_t length,
                                       uint8_t *connect_id, uint16_t *mid)
{
  return MqttParseCommandAck(response, length, "+MQTTPUB:", 3U, connect_id, mid);
}

ML307_Result ML307_MqttParseStateResponse(const uint8_t *response, size_t length,
                                         uint32_t *state)
{
  uint32_t value;
  ML307_Result result;
  if (state == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  result = MqttParseResponseFields(response, length, "+MQTTSTATE:", &value, 1U);
  if (result == ML307_RESULT_OK) *state = value;
  return result;
}

ML307_Result ML307_MqttParseConnectionResponse(const uint8_t *response,
                                              size_t length, uint8_t connect_id,
                                              uint32_t *state)
{
  const uint8_t *data;
  size_t line_length;
  size_t offset = 0U;
  ML307_Result result = ML307_RESULT_NOT_FOUND;
  if (response == NULL || state == NULL || connect_id > 5U)
    return ML307_RESULT_INVALID_ARGUMENT;
  while (ModuleFrameParser_NextLine(response, length, &offset, &data, &line_length)) {
    MqttControl control;
    if (memchr(data, '\0', line_length) == NULL &&
        MqttParseControl((AT_Line){(const char *)data, line_length}, &control) ==
            ML307_RESULT_OK &&
        control.type == ML307_MQTT_EVENT_CONNECTION &&
        control.fields[0] == connect_id) {
      *state = control.fields[1];
      result = ML307_RESULT_OK;
    }
  }
  return result;
}

int ML307_MqttResponseHasError(const char *raw)
{
  return AT_HasErrorResult(raw);
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

/* Text mode only. The caller has already established a record boundary. */
ML307_Result ML307_MqttParsePublishUrc(const uint8_t *line, size_t length,
                                      ML307_MqttEvent *event)
{
  static const char prefix[] = "+MQTTURC: \"publish\",";
  const uint8_t *cursor, *end, *topic;
  uint32_t cid, mid, total, fragment;
  size_t topic_length;
  if (event == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  memset(event, 0, sizeof(*event));
  if (line == NULL) return ML307_RESULT_INVALID_ARGUMENT;
  if (memchr(line, '\0', length) != NULL) return ML307_RESULT_INVALID_VALUE;
  cursor = line;
  end = line + length;
  while (cursor < end && (*cursor == ' ' || *cursor == '\t')) ++cursor;
  /* Remove framing only, never trim opaque payload spaces. */
  if (end > cursor && end[-1] == '\n') {
    --end;
    if (end > cursor && end[-1] == '\r') --end;
  }
  if ((size_t)(end - cursor) < sizeof(prefix) - 1U ||
      memcmp(cursor, prefix, sizeof(prefix) - 1U) != 0)
    return ML307_RESULT_NOT_FOUND;
  if (memchr(cursor, '\r', (size_t)(end - cursor)) != NULL ||
      memchr(cursor, '\n', (size_t)(end - cursor)) != NULL)
    return ML307_RESULT_INVALID_VALUE;
  cursor += sizeof(prefix) - 1U;
  if (ModuleFrameParser_ParseUnsigned(&cursor, end, 5U, &cid) != MODULE_FRAME_COMPLETE ||
      cursor == end || *cursor++ != ',' ||
      ModuleFrameParser_ParseUnsigned(&cursor, end, UINT16_MAX, &mid) != MODULE_FRAME_COMPLETE ||
      cursor == end || *cursor++ != ',' || cursor == end || *cursor++ != '"')
    return ML307_RESULT_INVALID_VALUE;
  topic = cursor;
  while (cursor < end && *cursor != '"') ++cursor;
  topic_length = (size_t)(cursor - topic);
  if (topic_length == 0U || cursor == end) return ML307_RESULT_INVALID_VALUE;
  if (topic_length >= sizeof(event->topic)) return ML307_RESULT_BUFFER_TOO_SMALL;
  ++cursor;
  if (cursor == end || *cursor++ != ',' ||
      ModuleFrameParser_ParseUnsigned(&cursor, end, UINT32_MAX, &total) != MODULE_FRAME_COMPLETE ||
      cursor == end || *cursor++ != ',' ||
      ModuleFrameParser_ParseUnsigned(&cursor, end, UINT32_MAX, &fragment) != MODULE_FRAME_COMPLETE ||
      cursor == end || *cursor++ != ',') return ML307_RESULT_INVALID_VALUE;
  if (fragment > total || (size_t)(end - cursor) != fragment)
    return ML307_RESULT_INVALID_VALUE;
  if (fragment >= sizeof(event->payload)) return ML307_RESULT_BUFFER_TOO_SMALL;
  /* Commit parsed fields only after every check has succeeded. */
  event->type = ML307_MQTT_EVENT_PUBLISH;
  event->connect_id = (uint8_t)cid;
  event->message_id = (uint16_t)mid;
  event->total_length = total;
  event->payload_length = fragment;
  memcpy(event->topic, topic, topic_length);
  memcpy(event->payload, cursor, fragment);
  return ML307_RESULT_OK;
}

ML307_Result ML307_MqttParseUrc(const char *raw, ML307_MqttEvent *event)
{
  const char *urc;
  size_t selected_length = 0U;
  char name[16];
  int connect_id;
  int value1 = 0;
  int value2 = 0;

  if ((raw == NULL) || (event == NULL)) {
    return ML307_RESULT_INVALID_ARGUMENT;
  }
  memset(event, 0, sizeof(*event));
  /* Select a line boundary, never a URC-looking substring in another record.
   * Keep the legacy NUL-terminated facade; bounded command scanners above
   * require LF so an incomplete UART response cannot complete a command.
   */
  urc = NULL;
  {
    const size_t length = strlen(raw);
    size_t offset = 0U;
    while (offset < length) {
      const uint8_t *data;
      size_t line_length;
      AT_Line line;
      ML307_Result result;
      if (!ModuleFrameParser_NextLine((const uint8_t *)raw, length, &offset,
                                      &data, &line_length)) {
        data = (const uint8_t *)raw + offset;
        line_length = length - offset;
        offset = length;
      }
      line = (AT_Line){(const char *)data, line_length};
      AT_TrimLine(&line);
      if (!AT_LineStartsWith(&line, "+MQTTURC:")) continue;
      result = ML307_MqttParseControlUrc(data, line_length, event);
      if (result != ML307_RESULT_NOT_FOUND) return result;
      /* Preserve trailing payload whitespace in the original selected line. */
      urc = line.data;
      selected_length = (size_t)(((const char *)data + line_length) - line.data);
      break;
    }
  }
  if ((urc == NULL) ||
      (sscanf(urc, "+MQTTURC: \"%15[^\"]\",%d", name, &connect_id) != 2) ||
      (connect_id < 0) || (connect_id > 5)) {
    return ML307_RESULT_NOT_FOUND;
  }
  event->connect_id = (uint8_t)connect_id;

  if (strcmp(name, "publish") == 0) {
    return ML307_MqttParsePublishUrc((const uint8_t *)urc, selected_length, event);
  }

  if (sscanf(urc, "+MQTTURC: \"%15[^\"]\",%d,%d,%d", name, &connect_id,
             &value1, &value2) < 3) {
    return ML307_RESULT_INVALID_VALUE;
  }
  event->message_id = (uint16_t)value1;
  event->state = value2;
  if (strcmp(name, "unsuback") == 0) {
    event->type = ML307_MQTT_EVENT_UNSUBACK;

  } else if (strcmp(name, "pubrec") == 0) {
    event->type = ML307_MQTT_EVENT_PUBREC;
  } else if (strcmp(name, "pubcomp") == 0) {
    event->type = ML307_MQTT_EVENT_PUBCOMP;

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

/* Audit-branch API retained as an alias to the single checked text parser. */
ML307_Result ML307_MqttParseTextPublish(const uint8_t *line, size_t length,
                                       ML307_MqttEvent *event)
{
  return ML307_MqttParsePublishUrc(line, length, event);
}
