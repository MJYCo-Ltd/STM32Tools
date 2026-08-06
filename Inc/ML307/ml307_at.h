/*
 ******************************************************************************
 * @file           : ml307_at.h
 * @brief          : Internal ML307 AT command builder (V2.0.5)
 *
 * Not for application use — prefer ML307_Pack / ML307_Unpack in ml307.h.
 ******************************************************************************
 */
#ifndef STM32TOOLS_ML307_AT_H
#define STM32TOOLS_ML307_AT_H

#include <stddef.h>
#include <stdint.h>

#include <ML307/ml307.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ML307_COMMAND_TERMINATOR "\r\n"

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

typedef enum {
  ML307_AT_FORM_TEST = 0,
  ML307_AT_FORM_READ,
  ML307_AT_FORM_SET,
  ML307_AT_FORM_EXECUTE
} ML307_AtForm;

typedef enum {
  ML307_CMD_KIND_BASIC = 0,
  ML307_CMD_KIND_PLUS
} ML307_CmdKind;

typedef enum {
  ML307_RESP_PLUS = 0,
  ML307_RESP_PLAIN
} ML307_RespStyle;

typedef enum {
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
  ML307_CMD_ATS0,
  ML307_CMD_ATA,
  ML307_CMD_ATD,
  ML307_CMD_ATH,
  ML307_CMD_CHUP,
  ML307_CMD_CEER,
  ML307_CMD_CRC,
  ML307_CMD_CREG,
  ML307_CMD_COPS,
  ML307_CMD_CLCK,
  ML307_CMD_CHLD,
  ML307_CMD_CLCC,
  ML307_CMD_CPOL,
  ML307_CMD_CPLS,
  ML307_CMD_COPN,
  ML307_CMD_CPAS,
  ML307_CMD_CFUN,
  ML307_CMD_CSQ,
  ML307_CMD_CESQ,
  ML307_CMD_CCLK,
  ML307_CMD_CLAC,
  ML307_CMD_CTZU,
  ML307_CMD_CTZR,
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
  ML307_CMD_CPIN,
  ML307_CMD_CPWD,
  ML307_CMD_CSIM,
  ML307_CMD_CRSM,
  ML307_CMD_CNUM,
  ML307_CMD_CIMI,
  ML307_CMD_CCHO,
  ML307_CMD_CCHC,
  ML307_CMD_CGLA,
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
  ML307_CMD_CMEE,
  ML307_CMD_COUNT
} ML307_Cmd;

typedef struct {
  ML307_Cmd id;
  ML307_Category category;
  ML307_CmdKind kind;
  ML307_AtForm default_query_form;
  ML307_RespStyle resp_style;
  const char *stem;
} ML307_CmdInfo;

const ML307_CmdInfo *ML307_GetCommands(size_t *count);
const ML307_CmdInfo *ML307_FindCommand(ML307_Cmd id);
int ML307_QueryIsSupported(ML307_Cmd command);
const char *ML307_GetExpectedResponseType(ML307_Cmd command);

ML307_Result ML307_FormatQueryHint(char *output, size_t output_size,
                                   ML307_Cmd command, const char *argument);
ML307_Result ML307_BuildAt(char *output, size_t output_size, ML307_Cmd command,
                           ML307_AtForm form, const char *argument);
ML307_Result ML307_BuildQuery(char *output, size_t output_size,
                              ML307_Cmd command);
ML307_Result ML307_BuildQueryCommand(char *output, size_t output_size,
                                     ML307_Cmd command, const char *argument);
ML307_Result ML307_BuildSleep(char *output, size_t output_size);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_ML307_AT_H */
