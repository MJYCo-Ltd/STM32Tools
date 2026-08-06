/*
 ******************************************************************************
 * @file           : ml307_parser.h
 * @brief          : ML307 AT response parser (no UART I/O)
 *
 * Matches AT Commands Reference Guide 4G Series V2.0.5 result codes:
 * OK / ERROR / +CME ERROR / +CMS ERROR, +TYPE: lines, and plain-text
 * identification responses (ATI, CIMI, CGMR, ...).
 ******************************************************************************
 */
#ifndef STM32TOOLS_ML307_PARSER_H
#define STM32TOOLS_ML307_PARSER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ML307_PARSED_TYPE_SIZE 32U
#define ML307_PARSED_INFO_SIZE 512U

typedef enum {
  ML307_PARSE_OK = 0,
  ML307_PARSE_INVALID_ARGUMENT,
  ML307_PARSE_NOT_FOUND,
  ML307_PARSE_ERROR_RESPONSE,
  ML307_PARSE_TRUNCATED
} ML307_ParseResult;

typedef struct {
  char type[ML307_PARSED_TYPE_SIZE];
  char info[ML307_PARSED_INFO_SIZE];
  uint8_t has_ok;
  uint8_t has_error;
  uint8_t truncated;
} ML307_ParsedResponse;

/** Non-zero when a final result code line is present. */
int ML307_ResponseIsComplete(const char *raw);

/**
 * Parse response for expected_type.
 *
 * - Plus responses: collect +expected_type: payloads (e.g. "CSQ", "CEREG").
 * - Plain responses: pass expected_type equal to stem ("ATI", "CIMI", "CGMR",
 *   "CGSN", ...) to collect non-+ body lines until OK/ERROR.
 */
ML307_ParseResult ML307_ParseResponse(const char *raw,
                                      const char *expected_type,
                                      ML307_ParsedResponse *out);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_ML307_PARSER_H */
