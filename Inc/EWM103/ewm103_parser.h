/*
 ******************************************************************************
 * @file           : ewm103_parser.h
 * @brief          : Internal EWM103 response helpers
 ******************************************************************************
 */
#ifndef STM32TOOLS_EWM103_PARSER_H
#define STM32TOOLS_EWM103_PARSER_H

#include <EWM103/ewm103.h>
#include <AT/ModuleFrameParser.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const ModuleLengthFrameProtocolEx EWM103_MqttReceiveProtocol;
typedef struct {
  uint32_t link;
  ModuleFrameSpan topic;
  ModuleFrameSpan payload;
} EWM103_MqttMessage;
typedef enum {
  EWM103_LINK_EVENT_NONE = 0,
  EWM103_LINK_EVENT_DISCONNECTED,
  EWM103_LINK_EVENT_GOT_IP,
  EWM103_LINK_EVENT_MQTT_DISCONNECTED
} EWM103_LinkEvent;
ModuleFrameResult EWM103_ParseMqttReceive(const uint8_t *frame, size_t length,
                                          EWM103_MqttMessage *message);
EWM103_LinkEvent EWM103_ParseLinkEvent(const uint8_t *line, size_t length);

int EWM103_ResponseIsComplete(const char *raw, EWM103_Type expect);
EWM103_Result EWM103_ParseResponse(const char *raw, EWM103_Type expect,
                                   EWM103_Data *out);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_EWM103_PARSER_H */
