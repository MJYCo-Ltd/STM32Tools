/*
 ******************************************************************************
 * @file           : ewm103_parser.h
 * @brief          : Internal EWM103 response helpers
 ******************************************************************************
 */
#ifndef STM32TOOLS_EWM103_PARSER_H
#define STM32TOOLS_EWM103_PARSER_H

#include <EWM103/ewm103.h>

#ifdef __cplusplus
extern "C" {
#endif

int EWM103_ResponseIsComplete(const char *raw, EWM103_Type expect);
EWM103_Result EWM103_ParseResponse(const char *raw, EWM103_Type expect,
                                   EWM103_Data *out);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_EWM103_PARSER_H */
