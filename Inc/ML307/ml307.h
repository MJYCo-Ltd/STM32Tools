/*
 ******************************************************************************
 * @file           : ml307.h
 * @brief          : ML307C DTU/RTU AT command builder
 ******************************************************************************
 */
#ifndef STM32TOOLS_ML307_H
#define STM32TOOLS_ML307_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ML307_COMMAND_TERMINATOR "\r\n"

/** Result of building an AT command. The output is empty on failure. */
typedef enum {
  ML307_RESULT_OK = 0,
  ML307_RESULT_INVALID_ARGUMENT,
  ML307_RESULT_INVALID_VALUE,
  ML307_RESULT_BUFFER_TOO_SMALL
} ML307_Result;

/** UART parity values defined by AT+UART. */
typedef enum {
  ML307_UART_PARITY_NONE = 0,
  ML307_UART_PARITY_ODD = 1,
  ML307_UART_PARITY_EVEN = 2
} ML307_UartParity;

/** Command groups follow the business chapters in the ML307C manual. */
typedef enum {
  ML307_QUERY_CATEGORY_BASIC = 0,
  ML307_QUERY_CATEGORY_DTU,
  ML307_QUERY_CATEGORY_SOCKET,
  ML307_QUERY_CATEGORY_MQTT,
  ML307_QUERY_CATEGORY_MAPPING,
  ML307_QUERY_CATEGORY_MCU_OTA,
  ML307_QUERY_CATEGORY_HTTP,
  ML307_QUERY_CATEGORY_UART,
  ML307_QUERY_CATEGORY_IO,
  ML307_QUERY_CATEGORY_GNSS
} ML307_QueryCategory;

/** Syntax used by a query entry. */
typedef enum {
  ML307_QUERY_SYNTAX_LITERAL = 0,          /**< ATI */
  ML307_QUERY_SYNTAX_SIMPLE,               /**< AT+NAME? */
  ML307_QUERY_SYNTAX_PARAMETER,            /**< AT+NAME=<argument> */
  ML307_QUERY_SYNTAX_PARAMETER_QUESTION,   /**< AT+NAME=<argument>? */
  ML307_QUERY_SYNTAX_PARAMETER_COMMA       /**< AT+NAME=<argument>, */
} ML307_QuerySyntax;

/** Query commands listed and grouped according to the manual. */
typedef enum {
  /* Basic commands. */
  ML307_QUERY_BASIC_VERSION = 0,
  ML307_QUERY_BASIC_IMEI,
  ML307_QUERY_BASIC_CSQ,
  ML307_QUERY_BASIC_ICCID,
  ML307_QUERY_BASIC_IMSI,
  ML307_QUERY_BASIC_SIM,
  ML307_QUERY_BASIC_SIM_MODE,
  ML307_QUERY_BASIC_SIM_INFO,
  ML307_QUERY_BASIC_CEREG,
  ML307_QUERY_BASIC_IS_LINK,
  ML307_QUERY_BASIC_UTC,
  ML307_QUERY_BASIC_TIME,
  ML307_QUERY_BASIC_NTP,
  ML307_QUERY_BASIC_CGI,
  ML307_QUERY_BASIC_LBS,
  ML307_QUERY_BASIC_APN,
  ML307_QUERY_BASIC_MONITOR,
  ML307_QUERY_BASIC_ONLINE_CFG,
  ML307_QUERY_BASIC_NET_LED,
  ML307_QUERY_BASIC_ADC,
  ML307_QUERY_BASIC_CFG_ID,

  /* DTU commands. */
  ML307_QUERY_DTU_TASK,
  ML307_QUERY_DTU_PSDN,
  ML307_QUERY_DTU_PSUP,
  ML307_QUERY_DTU_HEART,
  ML307_QUERY_DTU_REG,
  ML307_QUERY_DTU_STATE,
  ML307_QUERY_DTU_STATE_IO,
  ML307_QUERY_DTU_MSG_HEAD,
  ML307_QUERY_DTU_AT_PASSWORD,
  ML307_QUERY_DTU_FILTER,

  /* Socket commands. */
  ML307_QUERY_SOCKET_CONFIG,
  ML307_QUERY_SOCKET_BACKUP,
  ML307_QUERY_SOCKET_SHORT,
  ML307_QUERY_SOCKET_KEEP,
  ML307_QUERY_SOCKET_OTHER,

  /* MQTT commands. */
  ML307_QUERY_MQTT_CONFIG,
  ML307_QUERY_MQTT_AUTH,
  ML307_QUERY_MQTT_PLATFORM,
  ML307_QUERY_MQTT_SUB,
  ML307_QUERY_MQTT_PUB,
  ML307_QUERY_MQTT_SHORT,
  ML307_QUERY_MQTT_WILL,
  ML307_QUERY_MQTT_OTHER,
  ML307_QUERY_MQTT_GET_SUB,

  /* Data-mapping commands. */
  ML307_QUERY_MAPPING_USER,
  ML307_QUERY_MAPPING_USER_HEX,

  /* MCU OTA commands. */
  ML307_QUERY_MCU_OTA,

  /* HTTP commands. */
  ML307_QUERY_HTTP_URL,
  ML307_QUERY_HTTP_CFG,
  ML307_QUERY_HTTP_SSL,
  ML307_QUERY_HTTP_RESP,
  ML307_QUERY_HTTP_RANGE,

  /* UART commands. */
  ML307_QUERY_UART_CONFIG,
  ML307_QUERY_UART_QUEUE,

  /* IO commands. */
  ML307_QUERY_IO_CFG,
  ML307_QUERY_IO_GET,
  ML307_QUERY_IO_LOOP_REPORT,
  ML307_QUERY_IO_CHANGE_REPORT,
  ML307_QUERY_IO_TEMPLATE,
  ML307_QUERY_IO_TEMPLATE_HEX,

  /* GNSS commands (ML307C-GC only). */
  ML307_QUERY_GNSS_CONFIG,
  ML307_QUERY_GNSS_LOCATION,
  ML307_QUERY_GNSS_REPORT,
  ML307_QUERY_GNSS_LAST,

  ML307_QUERY_COMMAND_COUNT
} ML307_QueryCommand;

/** One entry in the documented query-command list. */
typedef struct {
  ML307_QueryCommand id;
  ML307_QueryCategory category;
  ML307_QuerySyntax syntax;
  const char *name;
} ML307_QueryCommandInfo;

/** Return the complete read-only query-command list. */
const ML307_QueryCommandInfo *ML307_GetQueryCommands(size_t *count);

/**
 * Build a command from the documented query list.
 *
 * argument must be NULL for LITERAL/SIMPLE entries. For the other syntax
 * types it supplies the part represented by <argument>, for example "1" or
 * "1,\"keepalive\"". No UART transfer or response processing is performed.
 */
ML307_Result ML307_BuildQueryCommand(char *output, size_t output_size,
                                     ML307_QueryCommand command,
                                     const char *argument);

/**
 * Build a literal AT command and append CRLF.
 *
 * command must start with "AT" and must not already contain CR or LF.
 * This function covers non-standard forms such as ATI and AT+SEND.
 */
ML307_Result ML307_BuildLiteral(char *output, size_t output_size,
                                const char *command);

/** Build AT+<name> followed by CRLF. */
ML307_Result ML307_BuildExecute(char *output, size_t output_size,
                                const char *name);

/** Build AT+<name>? followed by CRLF. */
ML307_Result ML307_BuildQuery(char *output, size_t output_size,
                              const char *name);

/** Build AT+<name>=<arguments> followed by CRLF. */
ML307_Result ML307_BuildSet(char *output, size_t output_size,
                            const char *name, const char *arguments);

/** Build AT+<name>=<arguments>? followed by CRLF (for indexed queries). */
ML307_Result ML307_BuildSetQuery(char *output, size_t output_size,
                                 const char *name, const char *arguments);

/** Build the basic AT attention command. */
ML307_Result ML307_BuildAttention(char *output, size_t output_size);

/** Build the ATI firmware-version query. */
ML307_Result ML307_BuildVersionQuery(char *output, size_t output_size);

/** Build AT+RESET. */
ML307_Result ML307_BuildReset(char *output, size_t output_size);

/** Build AT+SIM?. */
ML307_Result ML307_BuildSimQuery(char *output, size_t output_size);

/** Build AT+SIM=<sim_id>; sim_id is 1 (main) or 2 (secondary). */
ML307_Result ML307_BuildSimSelect(char *output, size_t output_size,
                                  uint8_t sim_id);

/** Build AT+UART=<id>?. id 1/2 maps to the module's UART0/UART1. */
ML307_Result ML307_BuildUartQuery(char *output, size_t output_size,
                                  uint8_t id);

/** Build AT+UART=<id>,<baud>,<data>,<parity>,<stop>. */
ML307_Result ML307_BuildUartConfig(char *output, size_t output_size,
                                   uint8_t id, uint32_t baudrate,
                                   uint8_t data_bits,
                                   ML307_UartParity parity,
                                   uint8_t stop_bits);

/** Build AT+UARTQUE=<id>?. */
ML307_Result ML307_BuildUartQueueQuery(char *output, size_t output_size,
                                       uint8_t id);

/** Build AT+UARTQUE=<id>,<count>,<size>,<latency>. */
ML307_Result ML307_BuildUartQueueConfig(char *output, size_t output_size,
                                        uint8_t id, uint8_t node_count,
                                        uint16_t node_size,
                                        uint16_t latency_ms);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_ML307_H */
