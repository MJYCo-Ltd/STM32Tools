#include <ML307/ml307.h>
#include <ML307/ml307_at.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

_Static_assert(ML307_TYPE_MQTT_PUBLISH == 12, "Existing facade values changed");
_Static_assert(ML307_TYPE_CPIN == 13 && ML307_TYPE_MCCID == 14,
               "SIM types must be appended");

static void TestQuery(ML307_Type type, const char *wire, const char *name)
{
  const ML307_Content content = {.type = type};
  char packet[32];
  size_t length = 99U;
  assert(ML307_Pack(&content, packet, sizeof(packet), &length) == ML307_RESULT_OK);
  assert(length == strlen(wire) && strcmp(packet, wire) == 0);
  assert(strcmp(ML307_TypeName(type), name) == 0);
  assert(ML307_Pack(&content, packet, strlen(wire), &length) ==
         ML307_RESULT_BUFFER_TOO_SMALL);
  assert(length == 0U && packet[0] == '\0');
}

static void TestResponses(void)
{
  static const char *const bad_iccid[] = {
    "\r\nOK\r\n", "\r\n+MCCID:\r\nOK\r\n",
    "+MCCID: 12345678901234567\r\nOK\r\n",
    "+MCCID: 12345678901234567890123\r\nOK\r\n",
    "+MCCID: 123456789012345678x\r\nOK\r\n",
    "+MCCID: 1234567890123456789012x\r\nOK\r\n",
    "+MCCID: 123456789012345678 9\r\nOK\r\n",
    "noise+MCCID: 123456789012345678\r\nOK\r\n",
    "+OTHER: +MCCID: 123456789012345678\r\nOK\r\n",
    "+MCCID: +MCCID: 123456789012345678\r\nOK\r\n",
    "+MCCID: 123456789012345678\r\nextra\r\nOK\r\n",
    "+MCCID: 123456789012345678\r\n+MCCID: 123456789012345678\r\nOK\r\n",
    "+MCCID: 123456789012345678\r\n",
    "+MCCID: 123456789012345678\r\nERROR\r\n",
    "+MCCID: 123456789012345678\r\nOK\r\nERROR\r\n",
    "+MCCID: 123456789012345678\r\n+CME ERROR: 10\r\n"
  };
  static const char *const bad_cpin[] = {
    "OK\r\n", "+CPIN:\r\nOK\r\n", "noise+CPIN: READY\r\nOK\r\n",
    "+CPIN: READY\r\n", "+CPIN: READY\r\nERROR\r\n",
    "+CPIN: READY\r\nOK\r\nERROR\r\n",
    "+CPIN: READY\r\n+CME ERROR: 10\r\n",
    "+CPIN: READY\r\nextra\r\nOK\r\n"
  };
  ML307_Data data;
  char oversized[600];
  size_t i;

  assert(ML307_Unpack("AT+CPIN?\r\n+CPIN: READY\r\nOK\r\n",
                      ML307_TYPE_CPIN, &data) == ML307_RESULT_OK);
  assert(data.ok && !data.error && strcmp(data.text, "READY") == 0);
  /* A valid non-ready response is decoded, never promoted to readiness. */
  assert(ML307_Unpack("+CPIN: SIM PIN\r\nOK\r\n", ML307_TYPE_CPIN, &data) ==
         ML307_RESULT_OK && strcmp(data.text, "SIM PIN") == 0);
  for (i = 0U; i < sizeof(bad_cpin) / sizeof(bad_cpin[0]); ++i)
    assert(ML307_Unpack(bad_cpin[i], ML307_TYPE_CPIN, &data) != ML307_RESULT_OK);
  for (i = 0U; i < sizeof(bad_iccid) / sizeof(bad_iccid[0]); ++i)
    assert(ML307_Unpack(bad_iccid[i], ML307_TYPE_MCCID, &data) != ML307_RESULT_OK);
  for (i = 18U; i <= 22U; i += 4U) {
    char reply[80];
    char identity[23];
    memset(identity, '8', i);
    identity[i] = '\0';
    snprintf(reply, sizeof(reply), "AT+MCCID\r\n+MCCID: %s\r\nOK\r\n", identity);
    assert(ML307_Unpack(reply, ML307_TYPE_MCCID, &data) == ML307_RESULT_OK);
    assert(data.ok && !data.error && strcmp(data.text, identity) == 0);
  }
  memcpy(oversized, "+MCCID: ", 8U);
  memset(oversized + 8U, '8', 550U);
  strcpy(oversized + 558U, "\r\nOK\r\n");
  assert(ML307_Unpack(oversized, ML307_TYPE_MCCID, &data) ==
         ML307_RESULT_BUFFER_TOO_SMALL);
  assert(!ML307_IsComplete("+CPIN: REA", ML307_TYPE_CPIN, 0U));
  assert(!ML307_IsComplete("+MCCID: 123456789012345678\r\nO",
                           ML307_TYPE_MCCID, 0U));
  assert(ML307_IsComplete("+CPIN: READY\r\nOK\r\n", ML307_TYPE_CPIN, 0U));
  assert(ML307_IsComplete("ERROR\r\n", ML307_TYPE_MCCID, 0U));
}

int main(void)
{
  TestQuery(ML307_TYPE_CPIN, "AT+CPIN?\r\n", "CPIN");
  TestQuery(ML307_TYPE_MCCID, "AT+MCCID\r\n", "MCCID");
  assert(ML307_FindCommand(ML307_CMD_CPIN)->default_query_form == ML307_AT_FORM_READ);
  assert(ML307_FindCommand(ML307_CMD_MCCID)->default_query_form == ML307_AT_FORM_EXECUTE);
  TestResponses();
  puts("ml307_sim_test: query packing and full SIM response validation passed");
  return 0;
}
