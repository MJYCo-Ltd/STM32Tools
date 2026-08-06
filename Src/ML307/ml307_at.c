/*
 ******************************************************************************
 * @file           : ml307_at.c
 * @brief          : Internal ML307C AT command builder (V2.0.5)
 ******************************************************************************
 */
#include "ML307/ml307_at.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define Q_EXEC ML307_AT_FORM_EXECUTE
#define Q_READ ML307_AT_FORM_READ
#define KIND_B ML307_CMD_KIND_BASIC
#define KIND_P ML307_CMD_KIND_PLUS
#define RESP_P ML307_RESP_PLUS
#define RESP_T ML307_RESP_PLAIN

static const ML307_CmdInfo s_commands[ML307_CMD_COUNT] = {
    /* 3. General */
    {ML307_CMD_ATE, ML307_CATEGORY_GENERAL, KIND_B, Q_EXEC, RESP_P, "ATE"},
    {ML307_CMD_ATS3, ML307_CATEGORY_GENERAL, KIND_B, Q_READ, RESP_T, "ATS3"},
    {ML307_CMD_ATS4, ML307_CATEGORY_GENERAL, KIND_B, Q_READ, RESP_T, "ATS4"},
    {ML307_CMD_ATS5, ML307_CATEGORY_GENERAL, KIND_B, Q_READ, RESP_T, "ATS5"},
    {ML307_CMD_AT_AND_F, ML307_CATEGORY_GENERAL, KIND_B, Q_EXEC, RESP_P, "AT&F"},
    {ML307_CMD_ATV, ML307_CATEGORY_GENERAL, KIND_B, Q_EXEC, RESP_P, "ATV"},
    {ML307_CMD_ATQ, ML307_CATEGORY_GENERAL, KIND_B, Q_EXEC, RESP_P, "ATQ"},
    {ML307_CMD_ATZ, ML307_CATEGORY_GENERAL, KIND_B, Q_EXEC, RESP_P, "ATZ"},
    {ML307_CMD_ATX, ML307_CATEGORY_GENERAL, KIND_B, Q_EXEC, RESP_P, "ATX"},
    {ML307_CMD_ATI, ML307_CATEGORY_GENERAL, KIND_B, Q_EXEC, RESP_T, "ATI"},
    {ML307_CMD_GMI, ML307_CATEGORY_GENERAL, KIND_P, Q_EXEC, RESP_T, "GMI"},
    {ML307_CMD_CGMI, ML307_CATEGORY_GENERAL, KIND_P, Q_EXEC, RESP_T, "CGMI"},
    {ML307_CMD_GMM, ML307_CATEGORY_GENERAL, KIND_P, Q_EXEC, RESP_T, "GMM"},
    {ML307_CMD_CGMM, ML307_CATEGORY_GENERAL, KIND_P, Q_EXEC, RESP_T, "CGMM"},
    {ML307_CMD_GMR, ML307_CATEGORY_GENERAL, KIND_P, Q_EXEC, RESP_T, "GMR"},
    {ML307_CMD_CGMR, ML307_CATEGORY_GENERAL, KIND_P, Q_EXEC, RESP_T, "CGMR"},
    {ML307_CMD_GSN, ML307_CATEGORY_GENERAL, KIND_P, Q_EXEC, RESP_T, "GSN"},
    {ML307_CMD_CGSN, ML307_CATEGORY_GENERAL, KIND_P, Q_EXEC, RESP_T, "CGSN"},
    {ML307_CMD_IPR, ML307_CATEGORY_GENERAL, KIND_P, Q_READ, RESP_P, "IPR"},
    {ML307_CMD_CSCS, ML307_CATEGORY_GENERAL, KIND_P, Q_READ, RESP_P, "CSCS"},

    /* 4. Call */
    {ML307_CMD_ATS0, ML307_CATEGORY_CALL, KIND_B, Q_READ, RESP_T, "ATS0"},
    {ML307_CMD_ATA, ML307_CATEGORY_CALL, KIND_B, Q_EXEC, RESP_P, "ATA"},
    {ML307_CMD_ATD, ML307_CATEGORY_CALL, KIND_B, Q_EXEC, RESP_P, "ATD"},
    {ML307_CMD_ATH, ML307_CATEGORY_CALL, KIND_B, Q_EXEC, RESP_P, "ATH"},
    {ML307_CMD_CHUP, ML307_CATEGORY_CALL, KIND_P, Q_EXEC, RESP_P, "CHUP"},
    {ML307_CMD_CEER, ML307_CATEGORY_CALL, KIND_P, Q_EXEC, RESP_P, "CEER"},
    {ML307_CMD_CRC, ML307_CATEGORY_CALL, KIND_P, Q_READ, RESP_P, "CRC"},

    /* 5. Network */
    {ML307_CMD_CREG, ML307_CATEGORY_NETWORK, KIND_P, Q_READ, RESP_P, "CREG"},
    {ML307_CMD_COPS, ML307_CATEGORY_NETWORK, KIND_P, Q_READ, RESP_P, "COPS"},
    {ML307_CMD_CLCK, ML307_CATEGORY_NETWORK, KIND_P, Q_EXEC, RESP_P, "CLCK"},
    {ML307_CMD_CHLD, ML307_CATEGORY_NETWORK, KIND_P, Q_EXEC, RESP_P, "CHLD"},
    {ML307_CMD_CLCC, ML307_CATEGORY_NETWORK, KIND_P, Q_EXEC, RESP_P, "CLCC"},
    {ML307_CMD_CPOL, ML307_CATEGORY_NETWORK, KIND_P, Q_READ, RESP_P, "CPOL"},
    {ML307_CMD_CPLS, ML307_CATEGORY_NETWORK, KIND_P, Q_READ, RESP_P, "CPLS"},
    {ML307_CMD_COPN, ML307_CATEGORY_NETWORK, KIND_P, Q_EXEC, RESP_P, "COPN"},

    /* 6. ME */
    {ML307_CMD_CPAS, ML307_CATEGORY_ME, KIND_P, Q_EXEC, RESP_P, "CPAS"},
    {ML307_CMD_CFUN, ML307_CATEGORY_ME, KIND_P, Q_READ, RESP_P, "CFUN"},
    {ML307_CMD_CSQ, ML307_CATEGORY_ME, KIND_P, Q_EXEC, RESP_P, "CSQ"},
    {ML307_CMD_CESQ, ML307_CATEGORY_ME, KIND_P, Q_EXEC, RESP_P, "CESQ"},
    {ML307_CMD_CCLK, ML307_CATEGORY_ME, KIND_P, Q_READ, RESP_P, "CCLK"},
    {ML307_CMD_CLAC, ML307_CATEGORY_ME, KIND_P, Q_EXEC, RESP_T, "CLAC"},
    {ML307_CMD_CTZU, ML307_CATEGORY_ME, KIND_P, Q_READ, RESP_P, "CTZU"},
    {ML307_CMD_CTZR, ML307_CATEGORY_ME, KIND_P, Q_READ, RESP_P, "CTZR"},

    /* 7. Packet */
    {ML307_CMD_CGDCONT, ML307_CATEGORY_PACKET, KIND_P, Q_READ, RESP_P, "CGDCONT"},
    {ML307_CMD_CGTFT, ML307_CATEGORY_PACKET, KIND_P, Q_READ, RESP_P, "CGTFT"},
    {ML307_CMD_CGATT, ML307_CATEGORY_PACKET, KIND_P, Q_READ, RESP_P, "CGATT"},
    {ML307_CMD_CGACT, ML307_CATEGORY_PACKET, KIND_P, Q_READ, RESP_P, "CGACT"},
    {ML307_CMD_CGPADDR, ML307_CATEGORY_PACKET, KIND_P, Q_EXEC, RESP_P, "CGPADDR"},
    {ML307_CMD_CGCLASS, ML307_CATEGORY_PACKET, KIND_P, Q_READ, RESP_P, "CGCLASS"},
    {ML307_CMD_CGEREP, ML307_CATEGORY_PACKET, KIND_P, Q_READ, RESP_P, "CGEREP"},
    {ML307_CMD_CGREG, ML307_CATEGORY_PACKET, KIND_P, Q_READ, RESP_P, "CGREG"},
    {ML307_CMD_CEREG, ML307_CATEGORY_PACKET, KIND_P, Q_READ, RESP_P, "CEREG"},
    {ML307_CMD_CGCONTRDP, ML307_CATEGORY_PACKET, KIND_P, Q_EXEC, RESP_P,
     "CGCONTRDP"},
    {ML307_CMD_CGEQOS, ML307_CATEGORY_PACKET, KIND_P, Q_READ, RESP_P, "CGEQOS"},
    {ML307_CMD_CGEQOSRDP, ML307_CATEGORY_PACKET, KIND_P, Q_EXEC, RESP_P,
     "CGEQOSRDP"},
    {ML307_CMD_CEMODE, ML307_CATEGORY_PACKET, KIND_P, Q_READ, RESP_P, "CEMODE"},
    {ML307_CMD_CGDEL, ML307_CATEGORY_PACKET, KIND_P, Q_EXEC, RESP_P, "CGDEL"},
    {ML307_CMD_CGAUTH, ML307_CATEGORY_PACKET, KIND_P, Q_READ, RESP_P, "CGAUTH"},

    /* 8. SIM */
    {ML307_CMD_CPIN, ML307_CATEGORY_SIM, KIND_P, Q_READ, RESP_P, "CPIN"},
    {ML307_CMD_CPWD, ML307_CATEGORY_SIM, KIND_P, Q_EXEC, RESP_P, "CPWD"},
    {ML307_CMD_CSIM, ML307_CATEGORY_SIM, KIND_P, Q_EXEC, RESP_P, "CSIM"},
    {ML307_CMD_CRSM, ML307_CATEGORY_SIM, KIND_P, Q_EXEC, RESP_P, "CRSM"},
    {ML307_CMD_CNUM, ML307_CATEGORY_SIM, KIND_P, Q_EXEC, RESP_P, "CNUM"},
    {ML307_CMD_CIMI, ML307_CATEGORY_SIM, KIND_P, Q_EXEC, RESP_T, "CIMI"},
    {ML307_CMD_CCHO, ML307_CATEGORY_SIM, KIND_P, Q_EXEC, RESP_P, "CCHO"},
    {ML307_CMD_CCHC, ML307_CATEGORY_SIM, KIND_P, Q_EXEC, RESP_P, "CCHC"},
    {ML307_CMD_CGLA, ML307_CATEGORY_SIM, KIND_P, Q_EXEC, RESP_P, "CGLA"},

    /* 9. SMS */
    {ML307_CMD_CSMS, ML307_CATEGORY_SMS, KIND_P, Q_READ, RESP_P, "CSMS"},
    {ML307_CMD_CMGF, ML307_CATEGORY_SMS, KIND_P, Q_READ, RESP_P, "CMGF"},
    {ML307_CMD_CSMP, ML307_CATEGORY_SMS, KIND_P, Q_READ, RESP_P, "CSMP"},
    {ML307_CMD_CSCA, ML307_CATEGORY_SMS, KIND_P, Q_READ, RESP_P, "CSCA"},
    {ML307_CMD_CSDH, ML307_CATEGORY_SMS, KIND_P, Q_READ, RESP_P, "CSDH"},
    {ML307_CMD_CNMI, ML307_CATEGORY_SMS, KIND_P, Q_READ, RESP_P, "CNMI"},
    {ML307_CMD_CMGR, ML307_CATEGORY_SMS, KIND_P, Q_EXEC, RESP_P, "CMGR"},
    {ML307_CMD_CMGC, ML307_CATEGORY_SMS, KIND_P, Q_EXEC, RESP_P, "CMGC"},
    {ML307_CMD_CMGL, ML307_CATEGORY_SMS, KIND_P, Q_EXEC, RESP_P, "CMGL"},
    {ML307_CMD_CMGD, ML307_CATEGORY_SMS, KIND_P, Q_EXEC, RESP_P, "CMGD"},
    {ML307_CMD_CMGW, ML307_CATEGORY_SMS, KIND_P, Q_EXEC, RESP_P, "CMGW"},
    {ML307_CMD_CMGS, ML307_CATEGORY_SMS, KIND_P, Q_EXEC, RESP_P, "CMGS"},
    {ML307_CMD_CMSS, ML307_CATEGORY_SMS, KIND_P, Q_EXEC, RESP_P, "CMSS"},
    {ML307_CMD_CPMS, ML307_CATEGORY_SMS, KIND_P, Q_READ, RESP_P, "CPMS"},
    {ML307_CMD_CMMS, ML307_CATEGORY_SMS, KIND_P, Q_READ, RESP_P, "CMMS"},

    /* 10. Error */
    {ML307_CMD_CMEE, ML307_CATEGORY_ERROR, KIND_P, Q_READ, RESP_P, "CMEE"},
};

_Static_assert(sizeof(s_commands) / sizeof(s_commands[0]) == ML307_CMD_COUNT,
               "ML307 command table and enum are out of sync");

static ML307_Result ML307_Fail(char *output, size_t output_size,
                               ML307_Result result)
{
  if ((output != NULL) && (output_size > 0U)) {
    output[0] = '\0';
  }
  return result;
}

static ML307_Result ML307_Format(char *output, size_t output_size,
                                 const char *fmt, ...)
{
  va_list args;
  int written;

  if ((output == NULL) || (output_size == 0U) || (fmt == NULL)) {
    return ML307_RESULT_INVALID_ARGUMENT;
  }

  va_start(args, fmt);
  written = vsnprintf(output, output_size, fmt, args);
  va_end(args);

  if (written < 0) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
  }
  if ((size_t)written >= output_size) {
    return ML307_Fail(output, output_size, ML307_RESULT_BUFFER_TOO_SMALL);
  }
  return ML307_RESULT_OK;
}

static int ML307_IsSingleLine(const char *text)
{
  if (text == NULL) {
    return 0;
  }
  while (*text != '\0') {
    if ((*text == '\r') || (*text == '\n')) {
      return 0;
    }
    ++text;
  }
  return 1;
}

static int ML307_FormAllowed(const ML307_CmdInfo *info, ML307_AtForm form)
{
  if (info->kind == ML307_CMD_KIND_PLUS) {
    return 1;
  }

  /* BASIC stems: no AT+NAME=? style for most; allow EXECUTE/SET/READ where
   * S-registers use ATS0? / ATS0=n. */
  switch (form) {
  case ML307_AT_FORM_EXECUTE:
    return 1;
  case ML307_AT_FORM_SET:
    return 1;
  case ML307_AT_FORM_READ:
    return (strncmp(info->stem, "ATS", 3) == 0);
  case ML307_AT_FORM_TEST:
    return 0;
  default:
    return 0;
  }
}

static ML307_Result ML307_BuildBasic(char *output, size_t output_size,
                                     const ML307_CmdInfo *info,
                                     ML307_AtForm form, const char *argument)
{
  switch (form) {
  case ML307_AT_FORM_EXECUTE:
    if (argument != NULL) {
      /* ATE0 / ATV1 / ATD... / AT&F0 */
      if (!ML307_IsSingleLine(argument)) {
        return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
      }
      return ML307_Format(output, output_size, "%s%s" ML307_COMMAND_TERMINATOR,
                          info->stem, argument);
    }
    return ML307_Format(output, output_size, "%s" ML307_COMMAND_TERMINATOR,
                        info->stem);

  case ML307_AT_FORM_SET:
    if ((argument == NULL) || !ML307_IsSingleLine(argument)) {
      return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
    }
    if (strncmp(info->stem, "ATS", 3) == 0) {
      return ML307_Format(output, output_size,
                          "%s=%s" ML307_COMMAND_TERMINATOR, info->stem,
                          argument);
    }
    return ML307_Format(output, output_size, "%s%s" ML307_COMMAND_TERMINATOR,
                        info->stem, argument);

  case ML307_AT_FORM_READ:
    return ML307_Format(output, output_size, "%s?" ML307_COMMAND_TERMINATOR,
                        info->stem);

  default:
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }
}

static ML307_Result ML307_BuildPlus(char *output, size_t output_size,
                                    const ML307_CmdInfo *info,
                                    ML307_AtForm form, const char *argument)
{
  switch (form) {
  case ML307_AT_FORM_TEST:
    return ML307_Format(output, output_size, "AT+%s=?" ML307_COMMAND_TERMINATOR,
                        info->stem);
  case ML307_AT_FORM_READ:
    return ML307_Format(output, output_size, "AT+%s?" ML307_COMMAND_TERMINATOR,
                        info->stem);
  case ML307_AT_FORM_EXECUTE:
    if (argument != NULL) {
      if (!ML307_IsSingleLine(argument)) {
        return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
      }
      return ML307_Format(output, output_size,
                          "AT+%s=%s" ML307_COMMAND_TERMINATOR, info->stem,
                          argument);
    }
    return ML307_Format(output, output_size, "AT+%s" ML307_COMMAND_TERMINATOR,
                        info->stem);
  case ML307_AT_FORM_SET:
    if ((argument == NULL) || !ML307_IsSingleLine(argument)) {
      return ML307_Fail(output, output_size, ML307_RESULT_INVALID_ARGUMENT);
    }
    return ML307_Format(output, output_size,
                        "AT+%s=%s" ML307_COMMAND_TERMINATOR, info->stem,
                        argument);
  default:
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }
}

const ML307_CmdInfo *ML307_GetCommands(size_t *count)
{
  if (count != NULL) {
    *count = (size_t)ML307_CMD_COUNT;
  }
  return s_commands;
}

const ML307_CmdInfo *ML307_FindCommand(ML307_Cmd id)
{
  if ((unsigned int)id >= (unsigned int)ML307_CMD_COUNT) {
    return NULL;
  }
  return &s_commands[(unsigned int)id];
}

int ML307_QueryIsSupported(ML307_Cmd command)
{
  return ML307_FindCommand(command) != NULL;
}

const char *ML307_GetExpectedResponseType(ML307_Cmd command)
{
  const ML307_CmdInfo *info = ML307_FindCommand(command);

  if (info == NULL) {
    return NULL;
  }
  return info->stem;
}

ML307_Result ML307_BuildAt(char *output, size_t output_size, ML307_Cmd command,
                           ML307_AtForm form, const char *argument)
{
  const ML307_CmdInfo *info = ML307_FindCommand(command);

  if (info == NULL) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }
  if (!ML307_FormAllowed(info, form)) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }

  if (info->kind == ML307_CMD_KIND_BASIC) {
    return ML307_BuildBasic(output, output_size, info, form, argument);
  }
  return ML307_BuildPlus(output, output_size, info, form, argument);
}

ML307_Result ML307_BuildQuery(char *output, size_t output_size,
                              ML307_Cmd command)
{
  const ML307_CmdInfo *info = ML307_FindCommand(command);

  if (info == NULL) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }
  return ML307_BuildAt(output, output_size, command, info->default_query_form,
                       NULL);
}

ML307_Result ML307_BuildQueryCommand(char *output, size_t output_size,
                                     ML307_Cmd command, const char *argument)
{
  const ML307_CmdInfo *info = ML307_FindCommand(command);

  if (info == NULL) {
    return ML307_Fail(output, output_size, ML307_RESULT_INVALID_VALUE);
  }

  /* NULL argument → default query form; non-NULL → SET / execute-with-arg. */
  if (argument == NULL) {
    return ML307_BuildQuery(output, output_size, command);
  }
  if (info->kind == ML307_CMD_KIND_PLUS) {
    return ML307_BuildAt(output, output_size, command, ML307_AT_FORM_SET,
                         argument);
  }
  return ML307_BuildAt(output, output_size, command, ML307_AT_FORM_EXECUTE,
                       argument);
}

ML307_Result ML307_FormatQueryHint(char *output, size_t output_size,
                                   ML307_Cmd command, const char *argument)
{
  char built[96];
  ML307_Result result;
  size_t length;

  result = ML307_BuildQueryCommand(built, sizeof(built), command, argument);
  if (result != ML307_RESULT_OK) {
    return ML307_Fail(output, output_size, result);
  }

  length = strlen(built);
  while ((length > 0U) &&
         ((built[length - 1U] == '\r') || (built[length - 1U] == '\n'))) {
    built[--length] = '\0';
  }
  return ML307_Format(output, output_size, "%s", built);
}

ML307_Result ML307_BuildSleep(char *output, size_t output_size)
{
  return ML307_Format(output, output_size,
                      "AT+MLPMCFG=\"sleepmode\",2,0" ML307_COMMAND_TERMINATOR);
}
