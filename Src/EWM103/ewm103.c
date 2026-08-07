/*
 ******************************************************************************
 * @file           : ewm103.c
 * @brief          : EWM103 pack / unpack facade
 ******************************************************************************
 */
#include "EWM103/ewm103.h"

#include "AT/at_codec.h"
#include "EWM103/ewm103_at.h"
#include "EWM103/ewm103_parser.h"

static AT_CodecResult ToCodecResult(EWM103_Result result)
{
  switch (result) {
  case EWM103_RESULT_OK:
    return AT_CODEC_OK;
  case EWM103_RESULT_BUFFER_TOO_SMALL:
    return AT_CODEC_BUFFER_TOO_SMALL;
  case EWM103_RESULT_INVALID_ARGUMENT:
    return AT_CODEC_INVALID_ARGUMENT;
  default:
    return AT_CODEC_FORMAT_ERROR;
  }
}

const char *EWM103_TypeName(EWM103_Type type)
{
  switch (type) {
  case EWM103_TYPE_AT:
    return "AT";
  case EWM103_TYPE_CMD:
    return "CMD";
  case EWM103_TYPE_RST:
    return "RST";
  case EWM103_TYPE_GMR:
    return "GMR";
  case EWM103_TYPE_ATE:
    return "ATE";
  case EWM103_TYPE_RESTORE:
    return "RESTORE";
  case EWM103_TYPE_UART_CUR:
    return "UART_CUR";
  case EWM103_TYPE_UART_DEF:
    return "UART_DEF";
  case EWM103_TYPE_SLEEPWKCFG:
    return "SLEEPWKCFG";
  case EWM103_TYPE_SLEEP:
    return "SLEEP";
  case EWM103_TYPE_CWMODE:
    return "CWMODE";
  case EWM103_TYPE_CWJAP:
    return "CWJAP";
  case EWM103_TYPE_CWLAP:
    return "CWLAP";
  case EWM103_TYPE_CWSAP:
    return "CWSAP";
  case EWM103_TYPE_CWLIF:
    return "CWLIF";
  case EWM103_TYPE_CWQAP:
    return "CWQAP";
  case EWM103_TYPE_CWDHCP:
    return "CWDHCP";
  case EWM103_TYPE_CWAUTOCONN:
    return "CWAUTOCONN";
  case EWM103_TYPE_CIPSTA:
    return "CIPSTA";
  case EWM103_TYPE_CIPAP:
    return "CIPAP";
  case EWM103_TYPE_CIPSTAMAC:
    return "CIPSTAMAC";
  case EWM103_TYPE_CIPAPMAC:
    return "CIPAPMAC";
  case EWM103_TYPE_CIPDOMAIN:
    return "CIPDOMAIN";
  case EWM103_TYPE_CIPSTART:
    return "CIPSTART";
  case EWM103_TYPE_CIPSTATUS:
    return "CIPSTATUS";
  case EWM103_TYPE_EXIT_TRANSPARENT:
    return "+++";
  case EWM103_TYPE_CIPSEND:
    return "CIPSEND";
  case EWM103_TYPE_CIPCLOSE:
    return "CIPCLOSE";
  case EWM103_TYPE_CIFSR:
    return "CIFSR";
  case EWM103_TYPE_CIPMUX:
    return "CIPMUX";
  case EWM103_TYPE_CIPSERVER:
    return "CIPSERVER";
  case EWM103_TYPE_CIPMODE:
    return "CIPMODE";
  case EWM103_TYPE_PING:
    return "PING";
  case EWM103_TYPE_CIPSNTPCFG:
    return "CIPSNTPCFG";
  case EWM103_TYPE_CIPSNTPTIME:
    return "CIPSNTPTIME";
  case EWM103_TYPE_CIPRECVMODE:
    return "CIPRECVMODE";
  case EWM103_TYPE_CIPRECVDATA:
    return "CIPRECVDATA";
  case EWM103_TYPE_CIPDINFO:
    return "CIPDINFO";
  case EWM103_TYPE_CIPRECVLEN:
    return "CIPRECVLEN";
  case EWM103_TYPE_CWDHCPS:
    return "CWDHCPS";
  case EWM103_TYPE_CIPSERVERMAXCONN:
    return "CIPSERVERMAXCONN";
  case EWM103_TYPE_CIPRECONNINTV:
    return "CIPRECONNINTV";
  case EWM103_TYPE_CIPRECVTYPE:
    return "CIPRECVTYPE";
  case EWM103_TYPE_MQTTUSERCFG:
    return "MQTTUSERCFG";
  case EWM103_TYPE_MQTTCONNCFG:
    return "MQTTCONNCFG";
  case EWM103_TYPE_MQTTCONN:
    return "MQTTCONN";
  case EWM103_TYPE_MQTTPUB:
    return "MQTTPUB";
  case EWM103_TYPE_MQTTPUBRAW:
    return "MQTTPUBRAW";
  case EWM103_TYPE_MQTTSUB:
    return "MQTTSUB";
  case EWM103_TYPE_MQTTUNSUB:
    return "MQTTUNSUB";
  case EWM103_TYPE_MQTTCLEAN:
    return "MQTTCLEAN";
  case EWM103_TYPE_WEBSERVER:
    return "WEBSERVER";
  case EWM103_TYPE_BLEPAIRSTART:
    return "BLEPAIRSTART";
  case EWM103_TYPE_BLEPAIRSTOP:
    return "BLEPAIRSTOP";
  default:
    return "?";
  }
}

EWM103_Result EWM103_Pack(const EWM103_Content *content, char *packet,
                          size_t packet_size, size_t *length)
{
  EWM103_Result result;
  AT_CodecResult finish_result;

  result = EWM103_AtBuild(content, packet, packet_size);
  finish_result = AT_FinishPacket(packet, packet_size, length,
                                  ToCodecResult(result));
  if (finish_result == AT_CODEC_BUFFER_TOO_SMALL) {
    return EWM103_RESULT_BUFFER_TOO_SMALL;
  }
  if (finish_result == AT_CODEC_INVALID_ARGUMENT) {
    return EWM103_RESULT_INVALID_ARGUMENT;
  }
  return result;
}

int EWM103_IsComplete(const char *packet, EWM103_Type expect)
{
  return EWM103_ResponseIsComplete(packet, expect);
}

EWM103_Result EWM103_Unpack(const char *packet, EWM103_Type expect,
                            EWM103_Data *out)
{
  return EWM103_ParseResponse(packet, expect, out);
}
