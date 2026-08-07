/*
 ******************************************************************************
 * @file           : at_codec.h
 * @brief          : Common helpers for packing and unpacking AT text protocols
 ******************************************************************************
 */
#ifndef STM32TOOLS_AT_CODEC_H
#define STM32TOOLS_AT_CODEC_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AT_CODEC_OK = 0,
  AT_CODEC_INVALID_ARGUMENT,
  AT_CODEC_FORMAT_ERROR,
  AT_CODEC_BUFFER_TOO_SMALL
} AT_CodecResult;

typedef struct {
  const char *data;
  size_t length;
} AT_Line;

/** Safe printf used by module-specific command packers; clears output on error. */
AT_CodecResult AT_Format(char *output, size_t output_size, const char *format,
                         ...);
AT_CodecResult AT_FormatV(char *output, size_t output_size, const char *format,
                          va_list args);

/** Finish a packed command and report its transmit length. */
AT_CodecResult AT_FinishPacket(char *packet, size_t packet_size,
                               size_t *length, AT_CodecResult result);

/** Validation shared by AT arguments that must remain on one command line. */
int AT_IsSingleLine(const char *text);

/** Read and trim one CR/LF-delimited response line. Returns next cursor. */
const char *AT_ReadLine(const char *cursor, AT_Line *line);
void AT_TrimLine(AT_Line *line);
int AT_LineEquals(const AT_Line *line, const char *token);
int AT_LineStartsWith(const AT_Line *line, const char *prefix);

/** Detect standard final result lines: OK, ERROR, +CME ERROR, +CMS ERROR. */
int AT_HasFinalResult(const char *response);

/** Bounded text operations for decoded response fields. */
void AT_CopyText(char *destination, size_t destination_size,
                 const char *source, size_t source_length,
                 uint8_t *truncated);
void AT_AppendLine(char *destination, size_t destination_size,
                   const char *line, size_t line_length,
                   uint8_t *truncated);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_AT_CODEC_H */
