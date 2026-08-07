/*
 ******************************************************************************
 * @file           : ewm103_parser.c
 * @brief          : EWM103-W15 response parser (AT manual V1.1)
 ******************************************************************************
 */
#include "EWM103/ewm103_parser.h"

#include "AT/at_codec.h"

#include <stddef.h>
#include <string.h>

static void CollectPlusLines(const char *raw, const char *prefix, char *out,
                             size_t out_size)
{
  const char *cursor = raw;
  const size_t prefix_len = (prefix != NULL) ? strlen(prefix) : 0U;

  out[0] = '\0';
  if ((raw == NULL) || (prefix == NULL)) {
    return;
  }
  while (*cursor != '\0') {
    AT_Line line;
    const char *payload;
    size_t payload_length;

    cursor = AT_ReadLine(cursor, &line);
    if ((line.length >= prefix_len) &&
        (strncmp(line.data, prefix, prefix_len) == 0)) {
      payload = line.data + prefix_len;
      payload_length = line.length - prefix_len;
      if ((payload_length > 0U) && (*payload == ':')) {
        ++payload;
        --payload_length;
      }
      while ((payload_length > 0U) &&
             ((*payload == ' ') || (*payload == '\t'))) {
        ++payload;
        --payload_length;
      }
      AT_AppendLine(out, out_size, payload, payload_length, NULL);
    }
  }
}

static void CollectNonAtBody(const char *raw, char *out, size_t out_size)
{
  const char *cursor = raw;

  out[0] = '\0';
  while ((cursor != NULL) && (*cursor != '\0')) {
    AT_Line line;

    cursor = AT_ReadLine(cursor, &line);
    if ((line.length > 0U) && !AT_LineStartsWith(&line, "AT") &&
        !AT_LineEquals(&line, "OK") && !AT_LineEquals(&line, "ERROR") &&
        !AT_LineStartsWith(&line, "+CME") &&
        !AT_LineStartsWith(&line, "+CMS")) {
      AT_AppendLine(out, out_size, line.data, line.length, NULL);
    }
  }
}

int EWM103_ResponseIsComplete(const char *raw, EWM103_Type expect)
{
  if (raw == NULL) {
    return 0;
  }
  /* Manual §5.2.5 / MQTT: wait for prompt or final SEND/MQTT result. */
  if ((expect == EWM103_TYPE_CIPSEND) || (expect == EWM103_TYPE_MQTTPUBRAW)) {
    if (AT_HasToken(raw, ">")) {
      return 1;
    }
    if (AT_HasToken(raw, "SEND OK") || AT_HasToken(raw, "SEND FAIL") ||
        AT_HasToken(raw, "+MQTTPUBRAW:OK") ||
        AT_HasToken(raw, "+MQTTPUBRAW:FAIL") ||
        AT_HasFinalResult(raw)) {
      return 1;
    }
    return 0;
  }
  /* Manual §6.2.2: +MQTTCONN:OK / +MQTTCONN:ERROR */
  if (expect == EWM103_TYPE_MQTTCONN) {
    return AT_HasToken(raw, "+MQTTCONN:OK") ||
           AT_HasToken(raw, "+MQTTCONN:ERROR") ||
           AT_HasFinalResult(raw);
  }
  /* Manual §5.2.4 +++ : ENTER AT MODE */
  if (expect == EWM103_TYPE_EXIT_TRANSPARENT) {
    return AT_HasToken(raw, "ENTER AT MODE") || AT_HasFinalResult(raw);
  }
  /* Manual §3.2.x / §4 / §5: final result line is OK or ERROR. */
  return AT_HasFinalResult(raw);
}

EWM103_Result EWM103_ParseResponse(const char *raw, EWM103_Type expect,
                                   EWM103_Data *out)
{
  if ((raw == NULL) || (out == NULL)) {
    return EWM103_RESULT_INVALID_ARGUMENT;
  }
  memset(out, 0, sizeof(*out));
  out->type = expect;
  out->value = -1;
  AT_CopyString(out->name, sizeof(out->name), EWM103_TypeName(expect), NULL);

  if (AT_HasErrorResult(raw) || AT_HasToken(raw, "SEND FAIL") ||
      AT_HasToken(raw, "+MQTTCONN:ERROR") ||
      AT_HasToken(raw, "+MQTTPUBRAW:FAIL")) {
    out->error = 1U;
    CollectNonAtBody(raw, out->text, sizeof(out->text));
    if (out->text[0] == '\0') {
      AT_CopyString(out->text, sizeof(out->text), "ERROR", NULL);
    }
    return EWM103_RESULT_ERROR_RESPONSE;
  }

  if ((expect == EWM103_TYPE_CIPSEND) || (expect == EWM103_TYPE_MQTTPUBRAW)) {
    if (AT_HasToken(raw, ">") && !AT_HasToken(raw, "SEND OK") &&
        !AT_HasToken(raw, "+MQTTPUBRAW:OK")) {
      out->need_payload = 1U;
      AT_CopyString(out->text, sizeof(out->text), ">", NULL);
      return EWM103_RESULT_NEED_PAYLOAD;
    }
  }

  out->ok = 1U;
  switch (expect) {
  case EWM103_TYPE_GMR:
    CollectNonAtBody(raw, out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CWMODE:
    CollectPlusLines(raw, "+CWMODE", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CWJAP:
    CollectPlusLines(raw, "+CWJAP", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CWLAP:
    CollectPlusLines(raw, "+CWLAP", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CWSAP:
    CollectPlusLines(raw, "+CWSAP", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CWLIF:
    CollectPlusLines(raw, "+CWLIF", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CWDHCP:
    CollectPlusLines(raw, "+CWDHCP", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPSTA:
    CollectPlusLines(raw, "+CIPSTA", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPAP:
    CollectPlusLines(raw, "+CIPAP", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPSTAMAC:
    CollectPlusLines(raw, "+CIPSTAMAC", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPAPMAC:
    CollectPlusLines(raw, "+CIPAPMAC", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIFSR:
    CollectPlusLines(raw, "+CIFSR", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPDOMAIN:
    CollectPlusLines(raw, "+CIPDOMAIN", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPSTATUS:
    CollectNonAtBody(raw, out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPMUX:
    CollectPlusLines(raw, "+CIPMUX", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPMODE:
    CollectPlusLines(raw, "+CIPMODE", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_PING:
    CollectPlusLines(raw, "+PING", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPSNTPCFG:
    CollectPlusLines(raw, "+CIPSNTPCFG", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPSNTPTIME:
    CollectPlusLines(raw, "+CIPSNTPTIME", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPRECVMODE:
    CollectPlusLines(raw, "+CIPRECVMODE", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPRECVDATA:
    CollectPlusLines(raw, "+CIPRECVDATA", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPRECVLEN:
    CollectPlusLines(raw, "+CIPRECVLEN", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CWDHCPS:
    CollectPlusLines(raw, "+CWDHCPS", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CIPSERVERMAXCONN:
    CollectPlusLines(raw, "+CIPSERVERMAXCONN", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_CMD:
    CollectPlusLines(raw, "+CMD", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_UART_CUR:
    CollectPlusLines(raw, "+UART_CUR", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_UART_DEF:
    CollectPlusLines(raw, "+UART_DEF", out->text, sizeof(out->text));
    break;
  case EWM103_TYPE_MQTTCONN:
    if (AT_HasToken(raw, "+MQTTCONN:OK")) {
      out->value = 1;
      AT_CopyString(out->text, sizeof(out->text), "connected", NULL);
    } else {
      CollectPlusLines(raw, "+MQTTCONN", out->text, sizeof(out->text));
    }
    break;
  case EWM103_TYPE_MQTTSUB:
    CollectPlusLines(raw, "+MQTTSUB", out->text, sizeof(out->text));
    break;
  default:
    CollectNonAtBody(raw, out->text, sizeof(out->text));
    if (out->text[0] == '\0') {
      AT_CopyString(out->text, sizeof(out->text), "OK", NULL);
    }
    break;
  }

  if (AT_HasToken(raw, "+IPD")) {
    CollectPlusLines(raw, "+IPD", out->payload, sizeof(out->payload));
  }
  if (AT_HasToken(raw, "+MQTTSUBRECV")) {
    CollectPlusLines(raw, "+MQTTSUBRECV", out->payload, sizeof(out->payload));
  }
  return EWM103_RESULT_OK;
}
