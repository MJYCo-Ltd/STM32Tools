/*
 ******************************************************************************
 * @file           : ml307.h
 * @brief          : ML307C AT command builder (no UART I/O)
 *
 * Command set follows China Mobile IoT
 * "AT Commands Reference Guide 4G Series V2.0.5".
 * Application owns UART TX/RX; use ml307_parser for response decoding.
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

typedef enum {
  ML307_RESULT_OK = 0,
  ML307_RESULT_INVALID_ARGUMENT,
  ML307_RESULT_INVALID_VALUE,
  ML307_RESULT_BUFFER_TOO_SMALL,
  ML307_RESULT_NOT_FOUND,
  ML307_RESULT_ERROR_RESPONSE
} ML307_Result;

/** Manual chapters 3–10. */
typedef enum {
  ML307_CATEGORY_GENERAL = 0,
  ML307_CATEGORY_CALL,
  ML307_CATEGORY_NETWORK,
  ML307_CATEGORY_ME,
  ML307_CATEGORY_PACKET,
  ML307_CATEGORY_SIM,
  ML307_CATEGORY_SMS,
  ML307_CATEGORY_ERROR
} ML307_Category;

/** AT command form (V2.0.5 Test / Read / Set / Execute). */
typedef enum {
  ML307_AT_FORM_TEST = 0, /**< AT+NAME=?  (basic: not used / invalid) */
  ML307_AT_FORM_READ,     /**< AT+NAME?   */
  ML307_AT_FORM_SET,      /**< AT+NAME=<arg> or basic ATE<arg> */
  ML307_AT_FORM_EXECUTE   /**< AT+NAME or ATI / ATA / ... */
} ML307_AtForm;

/** How the stem is formatted on the wire. */
typedef enum {
  ML307_CMD_KIND_BASIC = 0, /**< Full stem: ATI, ATE, ATS0, AT&F, ... */
  ML307_CMD_KIND_PLUS       /**< Stem is NAME in AT+NAME */
} ML307_CmdKind;

/** Default query response parsing style. */
typedef enum {
  ML307_RESP_PLUS = 0, /**< Expect +TYPE: payload lines */
  ML307_RESP_PLAIN     /**< Expect plain text body (ATI / CIMI / CGMR ...) */
} ML307_RespStyle;

/**
 * Full command list from V2.0.5 (chapters 3–10, excluding +++ escape).
 * Names match the manual section titles.
 */
typedef enum {
  /* 3. General */
  ML307_CMD_ATE = 0,
  ML307_CMD_ATS3,
  ML307_CMD_ATS4,
  ML307_CMD_ATS5,
  ML307_CMD_AT_AND_F,
  ML307_CMD_ATV,
  ML307_CMD_ATQ,
  ML307_CMD_ATZ,
  ML307_CMD_ATX,
  ML307_CMD_ATI,
  ML307_CMD_GMI,
  ML307_CMD_CGMI,
  ML307_CMD_GMM,
  ML307_CMD_CGMM,
  ML307_CMD_GMR,
  ML307_CMD_CGMR,
  ML307_CMD_GSN,
  ML307_CMD_CGSN,
  ML307_CMD_IPR,
  ML307_CMD_CSCS,

  /* 4. Call */
  ML307_CMD_ATS0,
  ML307_CMD_ATA,
  ML307_CMD_ATD,
  ML307_CMD_ATH,
  ML307_CMD_CHUP,
  ML307_CMD_CEER,
  ML307_CMD_CRC,

  /* 5. Network */
  ML307_CMD_CREG,
  ML307_CMD_COPS,
  ML307_CMD_CLCK,
  ML307_CMD_CHLD,
  ML307_CMD_CLCC,
  ML307_CMD_CPOL,
  ML307_CMD_CPLS,
  ML307_CMD_COPN,

  /* 6. ME */
  ML307_CMD_CPAS,
  ML307_CMD_CFUN,
  ML307_CMD_CSQ,
  ML307_CMD_CESQ,
  ML307_CMD_CCLK,
  ML307_CMD_CLAC,
  ML307_CMD_CTZU,
  ML307_CMD_CTZR,

  /* 7. Packet */
  ML307_CMD_CGDCONT,
  ML307_CMD_CGTFT,
  ML307_CMD_CGATT,
  ML307_CMD_CGACT,
  ML307_CMD_CGPADDR,
  ML307_CMD_CGCLASS,
  ML307_CMD_CGEREP,
  ML307_CMD_CGREG,
  ML307_CMD_CEREG,
  ML307_CMD_CGCONTRDP,
  ML307_CMD_CGEQOS,
  ML307_CMD_CGEQOSRDP,
  ML307_CMD_CEMODE,
  ML307_CMD_CGDEL,
  ML307_CMD_CGAUTH,

  /* 8. SIM */
  ML307_CMD_CPIN,
  ML307_CMD_CPWD,
  ML307_CMD_CSIM,
  ML307_CMD_CRSM,
  ML307_CMD_CNUM,
  ML307_CMD_CIMI,
  ML307_CMD_CCHO,
  ML307_CMD_CCHC,
  ML307_CMD_CGLA,

  /* 9. SMS */
  ML307_CMD_CSMS,
  ML307_CMD_CMGF,
  ML307_CMD_CSMP,
  ML307_CMD_CSCA,
  ML307_CMD_CSDH,
  ML307_CMD_CNMI,
  ML307_CMD_CMGR,
  ML307_CMD_CMGC,
  ML307_CMD_CMGL,
  ML307_CMD_CMGD,
  ML307_CMD_CMGW,
  ML307_CMD_CMGS,
  ML307_CMD_CMSS,
  ML307_CMD_CPMS,
  ML307_CMD_CMMS,

  /* 10. Error */
  ML307_CMD_CMEE,

  ML307_CMD_COUNT
} ML307_Cmd;

/** One entry in the V2.0.5 command table. */
typedef struct {
  ML307_Cmd id;
  ML307_Category category;
  ML307_CmdKind kind;
  ML307_AtForm default_query_form;
  ML307_RespStyle resp_style;
  const char *stem; /**< BASIC: "ATI"; PLUS: "CSQ" */
} ML307_CmdInfo;

const ML307_CmdInfo *ML307_GetCommands(size_t *count);
const ML307_CmdInfo *ML307_FindCommand(ML307_Cmd id);

/** Nonzero when id is in range (full V2.0.5 table is supported). */
int ML307_QueryIsSupported(ML307_Cmd command);

/**
 * Parser expected_type token: "CSQ", "CEREG", "ATI", "CIMI", ...
 * NULL if id invalid.
 */
const char *ML307_GetExpectedResponseType(ML307_Cmd command);

/** Short hint without CRLF, e.g. "ATI", "AT+CEREG?", "AT+CSQ". */
ML307_Result ML307_FormatQueryHint(char *output, size_t output_size,
                                   ML307_Cmd command, const char *argument);

/**
 * Build AT with explicit form. SET/ATD require argument; others usually NULL.
 * Output always includes CRLF on success.
 */
ML307_Result ML307_BuildAt(char *output, size_t output_size, ML307_Cmd command,
                           ML307_AtForm form, const char *argument);

/**
 * Build the default query form for this command (see CmdInfo.default_query_form).
 * Equivalent to ML307_BuildAt(..., default_query_form, NULL) for query-only
 * commands; SET-default commands still require ML307_BuildAt with argument.
 */
ML307_Result ML307_BuildQuery(char *output, size_t output_size,
                              ML307_Cmd command);

/** Alias of ML307_BuildQuery for call sites that pass a NULL argument. */
ML307_Result ML307_BuildQueryCommand(char *output, size_t output_size,
                                     ML307_Cmd command, const char *argument);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_ML307_H */
