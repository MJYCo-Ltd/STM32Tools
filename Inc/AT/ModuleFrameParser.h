#ifndef STM32TOOLS_MODULE_FRAME_PARSER_H
#define STM32TOOLS_MODULE_FRAME_PARSER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  MODULE_FRAME_INVALID = -1,
  MODULE_FRAME_INCOMPLETE = 0,
  MODULE_FRAME_COMPLETE = 1
} ModuleFrameResult;

typedef ModuleFrameResult (*ModuleFrameTailParser)(
    const uint8_t *tail, size_t available, size_t *consumed);

typedef void (*ModuleFrameLineCallback)(char *line, size_t length,
                                        void *context);

typedef struct {
  char *buffer;
  size_t capacity;
  size_t length;
  uint8_t discarding;
  uint8_t preserve_terminator; /* 0: trimmed line; 1: retain CR/LF */
  ModuleFrameLineCallback callback;
  void *context;
} ModuleLineCollector;

/** Hardware-specific description of a line-prefixed, length-delimited frame. */
typedef struct {
  const char *prefix;
  uint8_t field_count;
  uint8_t payload_length_field;
  uint8_t field_separator;
  uint8_t alternate_payload_separator;
  uint32_t maximum_payload_length;
  const char *fixed_tail;
  size_t fixed_tail_length;
  ModuleFrameTailParser parse_tail;
} ModuleLengthFrameProtocol;

uint8_t ModuleFrameParser_NextLine(const uint8_t *data, size_t length,
                                   size_t *offset, const uint8_t **line,
                                   size_t *line_length);

const uint8_t *ModuleFrameParser_FindLinePrefix(const uint8_t *data,
                                                size_t length,
                                                const char *prefix);

uint8_t ModuleFrameParser_HasLine(const uint8_t *data, size_t length,
                                  const char *value, uint8_t prefix_match);

ModuleFrameResult ModuleFrameParser_ParseUnsigned(const uint8_t **cursor,
                                                   const uint8_t *end,
                                                   uint32_t maximum,
                                                   uint32_t *value);

void ModuleFrameParser_InitLineCollector(ModuleLineCollector *collector,
                                         char *buffer, size_t capacity,
                                         ModuleFrameLineCallback callback,
                                         void *context);

/** Feed arbitrary UART bursts and emit only complete, CR/LF-trimmed lines. */
void ModuleFrameParser_FeedLines(ModuleLineCollector *collector,
                                 const uint8_t *data, size_t length);

/** Preserve CR/LF bytes for compatibility with line-oriented module adapters.
 * Overflow/NUL discard the WHOLE record through LF; no suffix is emitted.
 */
void ModuleFrameParser_FeedRawLines(ModuleLineCollector *collector,
                                    const uint8_t *data, size_t length);

/**
 * Parse one hardware frame without inspecting its binary payload.
 * Numeric fields are comma-like separated; the last separator introduces the
 * opaque body whose size is selected by payload_length_field.
 */
ModuleFrameResult ModuleFrameParser_ParseLengthFrame(
    const uint8_t *data, size_t length,
    const ModuleLengthFrameProtocol *protocol,
    uint32_t *fields, size_t fields_capacity,
    const uint8_t **payload, size_t *payload_length, size_t *consumed);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_MODULE_FRAME_PARSER_H */
