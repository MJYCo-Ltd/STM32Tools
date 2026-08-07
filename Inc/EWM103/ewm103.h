/*
 ******************************************************************************
 * @file           : ewm103.h
 * @brief          : EWM103-W15 AT pack / unpack facade (no UART I/O)
 *
 * Based on EWM103-W15 AT Command Manual V1.1.
 ******************************************************************************
 */
#ifndef STM32TOOLS_EWM103_H
#define STM32TOOLS_EWM103_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EWM103_DATA_NAME_SIZE 32U
#define EWM103_DATA_TEXT_SIZE 512U
#define EWM103_TOPIC_SIZE 128U
#define EWM103_PAYLOAD_SIZE 256U

typedef enum {
  EWM103_RESULT_OK = 0,
  EWM103_RESULT_INVALID_ARGUMENT,
  EWM103_RESULT_INVALID_VALUE,
  EWM103_RESULT_BUFFER_TOO_SMALL,
  EWM103_RESULT_NOT_FOUND,
  EWM103_RESULT_ERROR_RESPONSE,
  EWM103_RESULT_NEED_PAYLOAD /**< waiting for '>' then app sends body */
} EWM103_Result;

typedef enum {
  /* System */
  EWM103_TYPE_AT = 0,
  EWM103_TYPE_CMD,
  EWM103_TYPE_RST,
  EWM103_TYPE_GMR,
  EWM103_TYPE_ATE,
  EWM103_TYPE_RESTORE,
  EWM103_TYPE_UART_CUR,
  EWM103_TYPE_UART_DEF,
  EWM103_TYPE_SLEEPWKCFG,
  EWM103_TYPE_SLEEP,

  /* Wi-Fi */
  EWM103_TYPE_CWMODE,
  EWM103_TYPE_CWJAP,
  EWM103_TYPE_CWLAP,
  EWM103_TYPE_CWSAP,
  EWM103_TYPE_CWLIF,
  EWM103_TYPE_CWQAP,
  EWM103_TYPE_CWDHCP,
  EWM103_TYPE_CWAUTOCONN,
  EWM103_TYPE_CIPSTA,
  EWM103_TYPE_CIPAP,
  EWM103_TYPE_CIPSTAMAC,
  EWM103_TYPE_CIPAPMAC,

  /* TCP/IP */
  EWM103_TYPE_CIPDOMAIN,
  EWM103_TYPE_CIPSTART,
  EWM103_TYPE_CIPSTATUS,
  EWM103_TYPE_EXIT_TRANSPARENT,
  EWM103_TYPE_CIPSEND,
  EWM103_TYPE_CIPCLOSE,
  EWM103_TYPE_CIFSR,
  EWM103_TYPE_CIPMUX,
  EWM103_TYPE_CIPSERVER,
  EWM103_TYPE_CIPMODE,
  EWM103_TYPE_PING,
  EWM103_TYPE_CIPSNTPCFG,
  EWM103_TYPE_CIPSNTPTIME,
  EWM103_TYPE_CIPRECVMODE,
  EWM103_TYPE_CIPRECVDATA,
  EWM103_TYPE_CIPDINFO,
  EWM103_TYPE_CIPRECVLEN,
  EWM103_TYPE_CWDHCPS,
  EWM103_TYPE_CIPSERVERMAXCONN,
  EWM103_TYPE_CIPRECONNINTV,
  EWM103_TYPE_CIPRECVTYPE,

  /* MQTT */
  EWM103_TYPE_MQTTUSERCFG,
  EWM103_TYPE_MQTTCONNCFG,
  EWM103_TYPE_MQTTCONN,
  EWM103_TYPE_MQTTPUB,
  EWM103_TYPE_MQTTPUBRAW,
  EWM103_TYPE_MQTTSUB,
  EWM103_TYPE_MQTTUNSUB,
  EWM103_TYPE_MQTTCLEAN,

  /* WEB / BLE */
  EWM103_TYPE_WEBSERVER,
  EWM103_TYPE_BLEPAIRSTART,
  EWM103_TYPE_BLEPAIRSTOP
} EWM103_Type;

/**
 * Content to pack. Unused fields may be 0 / NULL.
 * query!=0 uses AT+NAME? when the command supports it.
 *
 * Common string mapping:
 *  s0: ssid / host / domain / topic / type("TCP") / mac / ip
 *  s1: pwd / remote_ip / username / path / gateway
 *  s2: bssid / client_id / netmask / SNTP server
 *  s3: password / path (MQTTUSERCFG) / SNTP server1
 *  s4: MQTT path / extra
 *  s5: MQTTUSERCFG path leftover / data for MQTTPUB
 */
typedef struct {
  EWM103_Type type;
  uint8_t query;
  uint8_t link_id;
  uint8_t mode;
  uint8_t flag;
  uint8_t qos;
  uint8_t retain;
  uint8_t mux; /**< 1: multi-link CIPSTART/CIPSEND form */
  uint16_t port;
  uint32_t length;
  uint32_t u0;
  uint32_t u1;
  uint32_t u2;
  uint32_t u3;
  const char *s0;
  const char *s1;
  const char *s2;
  const char *s3;
  const char *s4;
  const char *s5;
} EWM103_Content;

typedef struct {
  EWM103_Type type;
  uint8_t ok;
  uint8_t error;
  uint8_t need_payload;
  int value;
  uint8_t link_id;
  uint32_t length;
  char name[EWM103_DATA_NAME_SIZE];
  char text[EWM103_DATA_TEXT_SIZE];
  char topic[EWM103_TOPIC_SIZE];
  char payload[EWM103_PAYLOAD_SIZE];
} EWM103_Data;

const char *EWM103_TypeName(EWM103_Type type);

EWM103_Result EWM103_Pack(const EWM103_Content *content, char *packet,
                          size_t packet_size, size_t *length);

int EWM103_IsComplete(const char *packet, EWM103_Type expect);

EWM103_Result EWM103_Unpack(const char *packet, EWM103_Type expect,
                            EWM103_Data *out);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_EWM103_H */
