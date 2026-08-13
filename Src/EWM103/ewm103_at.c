/*
 ******************************************************************************
 * @file           : ewm103_at.c
 * @brief          : EWM103-W15 AT command builder
 ******************************************************************************
 */
#include "EWM103/ewm103_at.h"

#include "AT/at_codec.h"

#include <stdarg.h>

static EWM103_Result Fail(char *packet, size_t packet_size, EWM103_Result r)
{
  if ((packet != NULL) && (packet_size > 0U)) {
    packet[0] = '\0';
  }
  return r;
}

static EWM103_Result Format(char *packet, size_t packet_size, const char *fmt,
                            ...)
{
  va_list args;
  AT_CodecResult result;

  va_start(args, fmt);
  result = AT_FormatV(packet, packet_size, fmt, args);
  va_end(args);

  switch (result) {
  case AT_CODEC_OK:
    return EWM103_RESULT_OK;
  case AT_CODEC_BUFFER_TOO_SMALL:
    return EWM103_RESULT_BUFFER_TOO_SMALL;
  case AT_CODEC_FORMAT_ERROR:
    return EWM103_RESULT_INVALID_VALUE;
  default:
    return EWM103_RESULT_INVALID_ARGUMENT;
  }
}

static const char *S(const char *value)
{
  return (value != NULL) ? value : "";
}

static EWM103_Result BuildExec(char *packet, size_t packet_size,
                               const char *stem)
{
  return Format(packet, packet_size, "AT+%s" EWM103_COMMAND_TERMINATOR, stem);
}

static EWM103_Result BuildQuery(char *packet, size_t packet_size,
                                const char *stem)
{
  return Format(packet, packet_size, "AT+%s?" EWM103_COMMAND_TERMINATOR, stem);
}

EWM103_Result EWM103_AtBuild(const EWM103_Content *c, char *packet,
                             size_t packet_size)
{
  if ((c == NULL) || (packet == NULL) || (packet_size == 0U)) {
    return EWM103_RESULT_INVALID_ARGUMENT;
  }

  switch (c->type) {
  case EWM103_TYPE_AT:
    return Format(packet, packet_size, "AT" EWM103_COMMAND_TERMINATOR);
  case EWM103_TYPE_CMD:
    /* 手册 §3.2.2：查询指令 AT+CMD?（执行 AT+CMD 会 ERROR） */
    return BuildQuery(packet, packet_size, "CMD");
  case EWM103_TYPE_RST:
    return BuildExec(packet, packet_size, "RST");
  case EWM103_TYPE_GMR:
    return BuildExec(packet, packet_size, "GMR");
  case EWM103_TYPE_ATE:
    return Format(packet, packet_size, "ATE%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)(c->mode != 0U ? 1U : 0U));
  case EWM103_TYPE_RESTORE:
    return BuildExec(packet, packet_size, "RESTORE");
  case EWM103_TYPE_UART_CUR:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "UART_CUR");
    }
    return Format(packet, packet_size,
                  "AT+UART_CUR=%lu,%lu,%lu,%lu,%lu" EWM103_COMMAND_TERMINATOR,
                  (unsigned long)c->u0, (unsigned long)c->u1,
                  (unsigned long)c->u2, (unsigned long)c->u3,
                  (unsigned long)c->mode);
  case EWM103_TYPE_UART_DEF:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "UART_DEF");
    }
    return Format(packet, packet_size,
                  "AT+UART_DEF=%lu,%lu,%lu,%lu,%lu" EWM103_COMMAND_TERMINATOR,
                  (unsigned long)c->u0, (unsigned long)c->u1,
                  (unsigned long)c->u2, (unsigned long)c->u3,
                  (unsigned long)c->mode);
  case EWM103_TYPE_SLEEPWKCFG:
    return Format(packet, packet_size,
                  "AT+SLEEPWKCFG=%u,%lu,%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->mode, (unsigned long)c->u0,
                  (unsigned int)c->flag);
  case EWM103_TYPE_SLEEP:
    return Format(packet, packet_size, "AT+SLEEP=%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)(c->mode != 0U ? c->mode : 1U));

  case EWM103_TYPE_CWMODE:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CWMODE");
    }
    return Format(packet, packet_size, "AT+CWMODE=%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->mode);
  case EWM103_TYPE_CWJAP:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CWJAP");
    }
    if ((c->s2 != NULL) && (c->s2[0] != '\0')) {
      return Format(packet, packet_size,
                    "AT+CWJAP=\"%s\",\"%s\",\"%s\"" EWM103_COMMAND_TERMINATOR,
                    S(c->s0), S(c->s1), S(c->s2));
    }
    return Format(packet, packet_size,
                  "AT+CWJAP=\"%s\",\"%s\"" EWM103_COMMAND_TERMINATOR, S(c->s0),
                  S(c->s1));
  case EWM103_TYPE_CWLAP:
    return BuildExec(packet, packet_size, "CWLAP");
  case EWM103_TYPE_CWSAP:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CWSAP");
    }
    return Format(packet, packet_size,
                  "AT+CWSAP=\"%s\",\"%s\",%lu,%u" EWM103_COMMAND_TERMINATOR,
                  S(c->s0), S(c->s1), (unsigned long)c->u0,
                  (unsigned int)c->mode);
  case EWM103_TYPE_CWLIF:
    return BuildExec(packet, packet_size, "CWLIF");
  case EWM103_TYPE_CWQAP:
    return BuildExec(packet, packet_size, "CWQAP");
  case EWM103_TYPE_CWDHCP:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CWDHCP");
    }
    return Format(packet, packet_size,
                  "AT+CWDHCP=%u,%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->mode, (unsigned int)c->flag);
  case EWM103_TYPE_CWAUTOCONN:
    return Format(packet, packet_size,
                  "AT+CWAUTOCONN=%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->mode);
  case EWM103_TYPE_CIPSTA:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CIPSTA");
    }
    return Format(packet, packet_size,
                  "AT+CIPSTA=\"%s\",\"%s\",\"%s\"" EWM103_COMMAND_TERMINATOR,
                  S(c->s0), S(c->s1), S(c->s2));
  case EWM103_TYPE_CIPAP:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CIPAP");
    }
    return Format(packet, packet_size,
                  "AT+CIPAP=\"%s\",\"%s\",\"%s\"" EWM103_COMMAND_TERMINATOR,
                  S(c->s0), S(c->s1), S(c->s2));
  case EWM103_TYPE_CIPSTAMAC:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CIPSTAMAC");
    }
    return Format(packet, packet_size,
                  "AT+CIPSTAMAC=\"%s\"" EWM103_COMMAND_TERMINATOR, S(c->s0));
  case EWM103_TYPE_CIPAPMAC:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CIPAPMAC");
    }
    return Format(packet, packet_size,
                  "AT+CIPAPMAC=\"%s\"" EWM103_COMMAND_TERMINATOR, S(c->s0));

  case EWM103_TYPE_CIPDOMAIN:
    /* Manual §5.2.1 示例：AT+CIPDOMAIN=www.baidu.com（无引号） */
    return Format(packet, packet_size,
                  "AT+CIPDOMAIN=%s" EWM103_COMMAND_TERMINATOR, S(c->s0));
  case EWM103_TYPE_CIPSTART:
    if (c->mux != 0U) {
      return Format(
          packet, packet_size,
          "AT+CIPSTART=%u,\"%s\",\"%s\",%u" EWM103_COMMAND_TERMINATOR,
          (unsigned int)c->link_id, S(c->s0), S(c->s1), (unsigned int)c->port);
    }
    return Format(packet, packet_size,
                  "AT+CIPSTART=\"%s\",\"%s\",%u" EWM103_COMMAND_TERMINATOR,
                  S(c->s0), S(c->s1), (unsigned int)c->port);
  case EWM103_TYPE_CIPSTATUS:
    return BuildExec(packet, packet_size, "CIPSTATUS");
  case EWM103_TYPE_EXIT_TRANSPARENT:
    if (packet_size < 4U) {
      return Fail(packet, packet_size, EWM103_RESULT_BUFFER_TOO_SMALL);
    }
    packet[0] = '+';
    packet[1] = '+';
    packet[2] = '+';
    packet[3] = '\0';
    return EWM103_RESULT_OK;
  case EWM103_TYPE_CIPSEND:
    if (c->mux != 0U) {
      return Format(packet, packet_size,
                    "AT+CIPSEND=%u,%lu" EWM103_COMMAND_TERMINATOR,
                    (unsigned int)c->link_id, (unsigned long)c->length);
    }
    return Format(packet, packet_size,
                  "AT+CIPSEND=%lu" EWM103_COMMAND_TERMINATOR,
                  (unsigned long)c->length);
  case EWM103_TYPE_CIPCLOSE:
    if (c->mux != 0U) {
      return Format(packet, packet_size,
                    "AT+CIPCLOSE=%u" EWM103_COMMAND_TERMINATOR,
                    (unsigned int)c->link_id);
    }
    return BuildExec(packet, packet_size, "CIPCLOSE");
  case EWM103_TYPE_CIFSR:
    return BuildExec(packet, packet_size, "CIFSR");
  case EWM103_TYPE_CIPMUX:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CIPMUX");
    }
    return Format(packet, packet_size, "AT+CIPMUX=%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->mode);
  case EWM103_TYPE_CIPSERVER:
    return Format(packet, packet_size,
                  "AT+CIPSERVER=%u,%u,\"%s\"" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->mode, (unsigned int)c->port,
                  (c->s0 != NULL && c->s0[0] != '\0') ? c->s0 : "TCP");
  case EWM103_TYPE_CIPMODE:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CIPMODE");
    }
    return Format(packet, packet_size,
                  "AT+CIPMODE=%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->mode);
  case EWM103_TYPE_PING:
    return Format(packet, packet_size,
                  "AT+PING=\"%s\"" EWM103_COMMAND_TERMINATOR, S(c->s0));
  case EWM103_TYPE_CIPSNTPCFG:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CIPSNTPCFG");
    }
    return Format(packet, packet_size,
                  "AT+CIPSNTPCFG=%u,%ld,\"%s\",\"%s\"" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->mode, (long)c->u0, S(c->s0), S(c->s1));
  case EWM103_TYPE_CIPSNTPTIME:
    return BuildQuery(packet, packet_size, "CIPSNTPTIME");
  case EWM103_TYPE_CIPRECVMODE:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CIPRECVMODE");
    }
    return Format(packet, packet_size,
                  "AT+CIPRECVMODE=%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->mode);
  case EWM103_TYPE_CIPRECVDATA:
    return Format(packet, packet_size,
                  "AT+CIPRECVDATA=%u,%lu" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->link_id, (unsigned long)c->length);
  case EWM103_TYPE_CIPDINFO:
    return Format(packet, packet_size,
                  "AT+CIPDINFO=%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->mode);
  case EWM103_TYPE_CIPRECVLEN:
    return BuildQuery(packet, packet_size, "CIPRECVLEN");
  case EWM103_TYPE_CWDHCPS:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CWDHCPS");
    }
    if (c->mode == 0U) {
      return Format(packet, packet_size, "AT+CWDHCPS=0" EWM103_COMMAND_TERMINATOR);
    }
    return Format(packet, packet_size,
                  "AT+CWDHCPS=1,%lu,\"%s\",\"%s\"" EWM103_COMMAND_TERMINATOR,
                  (unsigned long)c->u0, S(c->s0), S(c->s1));
  case EWM103_TYPE_CIPSERVERMAXCONN:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CIPSERVERMAXCONN");
    }
    return Format(packet, packet_size,
                  "AT+CIPSERVERMAXCONN=%lu" EWM103_COMMAND_TERMINATOR,
                  (unsigned long)c->u0);
  case EWM103_TYPE_CIPRECONNINTV:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CIPRECONNINTV");
    }
    return Format(packet, packet_size,
                  "AT+CIPRECONNINTV=%lu" EWM103_COMMAND_TERMINATOR,
                  (unsigned long)c->u0);
  case EWM103_TYPE_CIPRECVTYPE:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "CIPRECVTYPE");
    }
    return Format(packet, packet_size,
                  "AT+CIPRECVTYPE=%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->mode);

  case EWM103_TYPE_MQTTUSERCFG:
    return Format(
        packet, packet_size,
        "AT+MQTTUSERCFG=%u,%u,\"%s\",\"%s\",\"%s\",%lu,%lu,\"%s\""
            EWM103_COMMAND_TERMINATOR,
        (unsigned int)c->link_id, (unsigned int)c->mode, S(c->s0), S(c->s1),
        S(c->s2), (unsigned long)c->u0, (unsigned long)c->u1, S(c->s3));
  case EWM103_TYPE_MQTTCONNCFG:
    return Format(packet, packet_size,
                  "AT+MQTTCONNCFG=%u,%lu,%lu,%lu,%lu,%lu" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->link_id, (unsigned long)c->u0,
                  (unsigned long)c->u1, (unsigned long)c->u2,
                  (unsigned long)c->u3, (unsigned long)c->mode);
  case EWM103_TYPE_MQTTCONN:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "MQTTCONN");
    }
    return Format(packet, packet_size,
                  "AT+MQTTCONN=%u,\"%s\",%u,%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->link_id, S(c->s0), (unsigned int)c->port,
                  (unsigned int)c->flag);
  case EWM103_TYPE_MQTTPUB:
    return Format(packet, packet_size,
                  "AT+MQTTPUB=%u,\"%s\",\"%s\",%u,%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->link_id, S(c->s0), S(c->s5),
                  (unsigned int)c->qos, (unsigned int)c->retain);
  case EWM103_TYPE_MQTTPUBRAW:
    return Format(
        packet, packet_size,
        "AT+MQTTPUBRAW=%u,\"%s\",%lu,%u,%u" EWM103_COMMAND_TERMINATOR,
        (unsigned int)c->link_id, S(c->s0), (unsigned long)c->length,
        (unsigned int)c->qos, (unsigned int)c->retain);
  case EWM103_TYPE_MQTTSUB:
    if (c->query != 0U) {
      return BuildQuery(packet, packet_size, "MQTTSUB");
    }
    return Format(packet, packet_size,
                  "AT+MQTTSUB=%u,\"%s\",%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->link_id, S(c->s0), (unsigned int)c->qos);
  case EWM103_TYPE_MQTTUNSUB:
    return Format(packet, packet_size,
                  "AT+MQTTUNSUB=%u,\"%s\"" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->link_id, S(c->s0));
  case EWM103_TYPE_MQTTCLEAN:
    return Format(packet, packet_size,
                  "AT+MQTTCLEAN=%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->link_id);

  case EWM103_TYPE_WEBSERVER:
    if (c->mode == 0U) {
      return Format(packet, packet_size,
                    "AT+WEBSERVER=0" EWM103_COMMAND_TERMINATOR);
    }
    return Format(packet, packet_size,
                  "AT+WEBSERVER=1,%u,%lu" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)c->port, (unsigned long)c->u0);
  case EWM103_TYPE_BLEINIT:
    /* 1=Server，0=释放协议栈 */
    return Format(packet, packet_size, "AT+BLEINIT=%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)(c->mode != 0U ? 1U : 0U));
  case EWM103_TYPE_BLUFI:
    /* 1=启动配网，0=关闭 */
    return Format(packet, packet_size, "AT+BLUFI=%u" EWM103_COMMAND_TERMINATOR,
                  (unsigned int)(c->mode != 0U ? 1U : 0U));
  default:
    return Fail(packet, packet_size, EWM103_RESULT_INVALID_VALUE);
  }
}
