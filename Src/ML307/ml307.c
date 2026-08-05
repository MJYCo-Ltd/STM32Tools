/*
 ******************************************************************************
 * @file           : ml307.c
 * @brief          : ML307C DTU/RTU AT command builder implementation
 ******************************************************************************
 */
#include "ML307/ml307.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const ML307_QueryCommandInfo s_query_commands[] = {
    /* Basic commands. */
    {ML307_QUERY_BASIC_VERSION, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_LITERAL, "ATI"},
    {ML307_QUERY_BASIC_IMEI, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "IMEI"},
    {ML307_QUERY_BASIC_CSQ, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "CSQ"},
    {ML307_QUERY_BASIC_ICCID, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "ICCID"},
    {ML307_QUERY_BASIC_IMSI, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "IMSI"},
    {ML307_QUERY_BASIC_SIM, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "SIM"},
    {ML307_QUERY_BASIC_SIM_MODE, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "SIMMODE"},
    {ML307_QUERY_BASIC_SIM_INFO, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "SIMINFO"},
    {ML307_QUERY_BASIC_CEREG, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "CEREG"},
    {ML307_QUERY_BASIC_IS_LINK, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "ISLINK"},
    {ML307_QUERY_BASIC_UTC, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "UTC"},
    {ML307_QUERY_BASIC_TIME, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "TIME"},
    {ML307_QUERY_BASIC_NTP, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "NTP"},
    {ML307_QUERY_BASIC_CGI, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "CGI"},
    {ML307_QUERY_BASIC_LBS, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "LBS"},
    {ML307_QUERY_BASIC_APN, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "APN"},
    {ML307_QUERY_BASIC_MONITOR, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "MONITOR"},
    {ML307_QUERY_BASIC_ONLINE_CFG, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "ONLINECFG"},
    {ML307_QUERY_BASIC_NET_LED, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "NETLED"},
    {ML307_QUERY_BASIC_ADC, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_PARAMETER, "ADC"},
    {ML307_QUERY_BASIC_CFG_ID, ML307_QUERY_CATEGORY_BASIC,
     ML307_QUERY_SYNTAX_SIMPLE, "CFGID"},

    /* DTU commands. */
    {ML307_QUERY_DTU_TASK, ML307_QUERY_CATEGORY_DTU,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "DTUTASK"},
    {ML307_QUERY_DTU_PSDN, ML307_QUERY_CATEGORY_DTU,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "DTUPSDN"},
    {ML307_QUERY_DTU_PSUP, ML307_QUERY_CATEGORY_DTU,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "DTUPSUP"},
    {ML307_QUERY_DTU_HEART, ML307_QUERY_CATEGORY_DTU,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "DTUHEART"},
    {ML307_QUERY_DTU_REG, ML307_QUERY_CATEGORY_DTU,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "DTUREG"},
    {ML307_QUERY_DTU_STATE, ML307_QUERY_CATEGORY_DTU,
     ML307_QUERY_SYNTAX_PARAMETER, "DTUSTATE"},
    {ML307_QUERY_DTU_STATE_IO, ML307_QUERY_CATEGORY_DTU,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "DTUSTATEIO"},
    {ML307_QUERY_DTU_MSG_HEAD, ML307_QUERY_CATEGORY_DTU,
     ML307_QUERY_SYNTAX_SIMPLE, "DTUMSGHEAD"},
    {ML307_QUERY_DTU_AT_PASSWORD, ML307_QUERY_CATEGORY_DTU,
     ML307_QUERY_SYNTAX_SIMPLE, "DTUATPSD"},
    {ML307_QUERY_DTU_FILTER, ML307_QUERY_CATEGORY_DTU,
     ML307_QUERY_SYNTAX_SIMPLE, "DTUFILTER"},

    /* Socket commands. */
    {ML307_QUERY_SOCKET_CONFIG, ML307_QUERY_CATEGORY_SOCKET,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "SOCK"},
    {ML307_QUERY_SOCKET_BACKUP, ML307_QUERY_CATEGORY_SOCKET,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "SOCKBAKE"},
    {ML307_QUERY_SOCKET_SHORT, ML307_QUERY_CATEGORY_SOCKET,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "SOCKSHORT"},
    {ML307_QUERY_SOCKET_KEEP, ML307_QUERY_CATEGORY_SOCKET,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "SOCKKEEP"},
    {ML307_QUERY_SOCKET_OTHER, ML307_QUERY_CATEGORY_SOCKET,
     ML307_QUERY_SYNTAX_PARAMETER_COMMA, "SOCKOTHER"},

    /* MQTT commands. */
    {ML307_QUERY_MQTT_CONFIG, ML307_QUERY_CATEGORY_MQTT,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "MQTT"},
    {ML307_QUERY_MQTT_AUTH, ML307_QUERY_CATEGORY_MQTT,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "MQTTAUTH"},
    {ML307_QUERY_MQTT_PLATFORM, ML307_QUERY_CATEGORY_MQTT,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "MQTTPLATFORM"},
    {ML307_QUERY_MQTT_SUB, ML307_QUERY_CATEGORY_MQTT,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "MQTTSUB"},
    {ML307_QUERY_MQTT_PUB, ML307_QUERY_CATEGORY_MQTT,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "MQTTPUB"},
    {ML307_QUERY_MQTT_SHORT, ML307_QUERY_CATEGORY_MQTT,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "MQTTSHORT"},
    {ML307_QUERY_MQTT_WILL, ML307_QUERY_CATEGORY_MQTT,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "MQTTWILL"},
    {ML307_QUERY_MQTT_OTHER, ML307_QUERY_CATEGORY_MQTT,
     ML307_QUERY_SYNTAX_PARAMETER_COMMA, "MQTTOTHER"},
    {ML307_QUERY_MQTT_GET_SUB, ML307_QUERY_CATEGORY_MQTT,
     ML307_QUERY_SYNTAX_PARAMETER, "MQTTGETSUB"},

    /* Data mapping and MCU OTA. */
    {ML307_QUERY_MAPPING_USER, ML307_QUERY_CATEGORY_MAPPING,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "MAPUSER"},
    {ML307_QUERY_MAPPING_USER_HEX, ML307_QUERY_CATEGORY_MAPPING,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "MAPUSERH"},
    {ML307_QUERY_MCU_OTA, ML307_QUERY_CATEGORY_MCU_OTA,
     ML307_QUERY_SYNTAX_SIMPLE, "MCUOTA"},

    /* HTTP commands. */
    {ML307_QUERY_HTTP_URL, ML307_QUERY_CATEGORY_HTTP,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "HTTPURL"},
    {ML307_QUERY_HTTP_CFG, ML307_QUERY_CATEGORY_HTTP,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "HTTPCFG"},
    {ML307_QUERY_HTTP_SSL, ML307_QUERY_CATEGORY_HTTP,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "HTTPSSL"},
    {ML307_QUERY_HTTP_RESP, ML307_QUERY_CATEGORY_HTTP,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "HTTPRESP"},
    {ML307_QUERY_HTTP_RANGE, ML307_QUERY_CATEGORY_HTTP,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "HTTPRANGE"},

    /* UART commands. */
    {ML307_QUERY_UART_CONFIG, ML307_QUERY_CATEGORY_UART,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "UART"},
    {ML307_QUERY_UART_QUEUE, ML307_QUERY_CATEGORY_UART,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "UARTQUE"},

    /* IO commands. */
    {ML307_QUERY_IO_CFG, ML307_QUERY_CATEGORY_IO,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "IOCFG"},
    {ML307_QUERY_IO_GET, ML307_QUERY_CATEGORY_IO,
     ML307_QUERY_SYNTAX_PARAMETER, "IOGET"},
    {ML307_QUERY_IO_LOOP_REPORT, ML307_QUERY_CATEGORY_IO,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "IOLR"},
    {ML307_QUERY_IO_CHANGE_REPORT, ML307_QUERY_CATEGORY_IO,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "IOCR"},
    {ML307_QUERY_IO_TEMPLATE, ML307_QUERY_CATEGORY_IO,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "IOTMPL"},
    {ML307_QUERY_IO_TEMPLATE_HEX, ML307_QUERY_CATEGORY_IO,
     ML307_QUERY_SYNTAX_PARAMETER_QUESTION, "IOTMPLH"},

    /* GNSS commands (ML307C-GC only). */
    {ML307_QUERY_GNSS_CONFIG, ML307_QUERY_CATEGORY_GNSS,
     ML307_QUERY_SYNTAX_SIMPLE, "GNSS"},
    {ML307_QUERY_GNSS_LOCATION, ML307_QUERY_CATEGORY_GNSS,
     ML307_QUERY_SYNTAX_PARAMETER, "GNSSLOC"},
    {ML307_QUERY_GNSS_REPORT, ML307_QUERY_CATEGORY_GNSS,
     ML307_QUERY_SYNTAX_SIMPLE, "GNSSREP"},
    {ML307_QUERY_GNSS_LAST, ML307_QUERY_CATEGORY_GNSS,
     ML307_QUERY_SYNTAX_SIMPLE, "GNSSLAST"},
};

_Static_assert((sizeof(s_query_commands) / sizeof(s_query_commands[0])) ==
                   ML307_QUERY_COMMAND_COUNT,
               "ML307 query list and enum are out of sync");

static ML307_Result ML307_Fail(char *output, size_t output_size,
                               ML307_Result result)
{
  if ((output != NULL) && (output_size > 0U)) {
    output[0] = '\0';
  }
  return result;
}

static ML307_Result ML307_Format(char *output, size_t output_size,
                                 const char *format, ...)
{
  int length;
  va_list args;

  if ((output == NULL) || (output_size == 0U) || (format == NULL)) {
    return ML307_RESULT_INVALID_ARGUMENT;
  }

  output[0] = '\0';
  va_start(args, format);
  length = vsnprintf(output, output_size, format, args);
  va_end(args);

  if (length < 0) {
    output[0] = '\0';
    return ML307_RESULT_INVALID_ARGUMENT;
  }
  if ((size_t)length >= output_size) {
    output[0] = '\0';
    return ML307_RESULT_BUFFER_TOO_SMALL;
  }
  return ML307_RESULT_OK;
}

static int ML307_IsCommandName(const char *name)
{
  const unsigned char *cursor = (const unsigned char *)name;

  if ((cursor == NULL) || (*cursor == '\0')) {
    return 0;
  }
  while (*cursor != '\0') {
    if ((isalnum(*cursor) == 0) && (*cursor != '_')) {
      return 0;
    }
    ++cursor;
  }
  return 1;
}

static int ML307_IsSingleLine(const char *text)
{
  return (text != NULL) && (text[0] != '\0') &&
         (strchr(text, '\r') == NULL) && (strchr(text, '\n') == NULL);
}

const ML307_QueryCommandInfo *ML307_GetQueryCommands(size_t *count)
{
  if (count != NULL) {
    *count = sizeof(s_query_commands) / sizeof(s_query_commands[0]);
  }
  return s_query_commands;
}

ML307_Result ML307_BuildQueryCommand(char *output, size_t output_size,
                                     ML307_QueryCommand command,
                                     const char *argument)
{
  const ML307_QueryCommandInfo *info;

  if ((unsigned int)command >= (unsigned int)ML307_QUERY_COMMAND_COUNT) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }

  info = &s_query_commands[(size_t)command];
  if ((info->syntax == ML307_QUERY_SYNTAX_LITERAL) ||
      (info->syntax == ML307_QUERY_SYNTAX_SIMPLE)) {
    if ((argument != NULL) && (argument[0] != '\0')) {
      return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
    }
  } else if (!ML307_IsSingleLine(argument)) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
  }

  switch (info->syntax) {
  case ML307_QUERY_SYNTAX_LITERAL:
    return ML307_BuildLiteral(output, output_size, info->name);
  case ML307_QUERY_SYNTAX_SIMPLE:
    return ML307_BuildQuery(output, output_size, info->name);
  case ML307_QUERY_SYNTAX_PARAMETER:
    return ML307_Format(output, output_size,
                        "AT+%s=%s" ML307_COMMAND_TERMINATOR, info->name,
                        argument);
  case ML307_QUERY_SYNTAX_PARAMETER_QUESTION:
    return ML307_BuildSetQuery(output, output_size, info->name, argument);
  case ML307_QUERY_SYNTAX_PARAMETER_COMMA:
    return ML307_Format(output, output_size,
                        "AT+%s=%s," ML307_COMMAND_TERMINATOR, info->name,
                        argument);
  default:
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }
}

ML307_Result ML307_BuildLiteral(char *output, size_t output_size,
                                const char *command)
{
  if (!ML307_IsSingleLine(command) || (command[0] != 'A') ||
      (command[1] != 'T')) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
  }
  return ML307_Format(output, output_size, "%s" ML307_COMMAND_TERMINATOR,
                      command);
}

ML307_Result ML307_BuildExecute(char *output, size_t output_size,
                                const char *name)
{
  if (!ML307_IsCommandName(name)) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
  }
  return ML307_Format(output, output_size,
                      "AT+%s" ML307_COMMAND_TERMINATOR, name);
}

ML307_Result ML307_BuildQuery(char *output, size_t output_size,
                              const char *name)
{
  if (!ML307_IsCommandName(name)) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
  }
  return ML307_Format(output, output_size,
                      "AT+%s?" ML307_COMMAND_TERMINATOR, name);
}

ML307_Result ML307_BuildSet(char *output, size_t output_size,
                            const char *name, const char *arguments)
{
  if (!ML307_IsCommandName(name) || !ML307_IsSingleLine(arguments)) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
  }
  return ML307_Format(output, output_size,
                      "AT+%s=%s" ML307_COMMAND_TERMINATOR, name, arguments);
}

ML307_Result ML307_BuildSetQuery(char *output, size_t output_size,
                                 const char *name, const char *arguments)
{
  if (!ML307_IsCommandName(name) || !ML307_IsSingleLine(arguments)) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
  }
  return ML307_Format(output, output_size,
                      "AT+%s=%s?" ML307_COMMAND_TERMINATOR, name, arguments);
}

ML307_Result ML307_BuildAttention(char *output, size_t output_size)
{
  return ML307_BuildLiteral(output, output_size, "AT");
}

ML307_Result ML307_BuildVersionQuery(char *output, size_t output_size)
{
  return ML307_BuildLiteral(output, output_size, "ATI");
}

ML307_Result ML307_BuildReset(char *output, size_t output_size)
{
  return ML307_BuildExecute(output, output_size, "RESET");
}

ML307_Result ML307_BuildSimQuery(char *output, size_t output_size)
{
  return ML307_BuildQuery(output, output_size, "SIM");
}

ML307_Result ML307_BuildSimSelect(char *output, size_t output_size,
                                  uint8_t sim_id)
{
  if ((sim_id < 1U) || (sim_id > 2U)) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }
  return ML307_Format(output, output_size,
                      "AT+SIM=%u" ML307_COMMAND_TERMINATOR,
                      (unsigned int)sim_id);
}

ML307_Result ML307_BuildUartQuery(char *output, size_t output_size,
                                  uint8_t id)
{
  if ((id < 1U) || (id > 2U)) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }
  return ML307_Format(output, output_size,
                      "AT+UART=%u?" ML307_COMMAND_TERMINATOR,
                      (unsigned int)id);
}

ML307_Result ML307_BuildUartConfig(char *output, size_t output_size,
                                   uint8_t id, uint32_t baudrate,
                                   uint8_t data_bits,
                                   ML307_UartParity parity,
                                   uint8_t stop_bits)
{
  if ((id < 1U) || (id > 2U) || (baudrate < 1200U) ||
      (baudrate > 115200U) || (data_bits < 5U) || (data_bits > 8U) ||
      (parity > ML307_UART_PARITY_EVEN) || (stop_bits < 1U) ||
      (stop_bits > 2U)) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }

  return ML307_Format(output, output_size,
                      "AT+UART=%u,%lu,%u,%u,%u" ML307_COMMAND_TERMINATOR,
                      (unsigned int)id, (unsigned long)baudrate,
                      (unsigned int)data_bits, (unsigned int)parity,
                      (unsigned int)stop_bits);
}

ML307_Result ML307_BuildUartQueueQuery(char *output, size_t output_size,
                                       uint8_t id)
{
  if ((id < 1U) || (id > 2U)) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }
  return ML307_Format(output, output_size,
                      "AT+UARTQUE=%u?" ML307_COMMAND_TERMINATOR,
                      (unsigned int)id);
}

ML307_Result ML307_BuildUartQueueConfig(char *output, size_t output_size,
                                        uint8_t id, uint8_t node_count,
                                        uint16_t node_size,
                                        uint16_t latency_ms)
{
  if ((id < 1U) || (id > 2U) || (node_count < 1U) ||
      (node_count > 32U) || (node_size < 32U) || (node_size > 8192U) ||
      (latency_ms < 10U) || (latency_ms > 1000U)) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }

  return ML307_Format(output, output_size,
                      "AT+UARTQUE=%u,%u,%u,%u" ML307_COMMAND_TERMINATOR,
                      (unsigned int)id, (unsigned int)node_count,
                      (unsigned int)node_size, (unsigned int)latency_ms);
}
