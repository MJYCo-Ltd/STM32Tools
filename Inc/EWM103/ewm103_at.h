/*
 ******************************************************************************
 * @file           : ewm103_at.h
 * @brief          : Internal EWM103 AT builder
 ******************************************************************************
 */
#ifndef STM32TOOLS_EWM103_AT_H
#define STM32TOOLS_EWM103_AT_H

#include <stddef.h>

#include <EWM103/ewm103.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EWM103_COMMAND_TERMINATOR "\r\n"

EWM103_Result EWM103_AtBuild(const EWM103_Content *content, char *packet,
                             size_t packet_size);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_EWM103_AT_H */
