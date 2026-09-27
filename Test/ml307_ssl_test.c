#include <ML307/ml307_ssl.h>

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

static void TestOptionCommands(void)
{
  char output[96];
  char short_output[8] = "dirty";
  CHECK(ML307_SslBuildSetOption(output, sizeof(output), ML307_SSL_OPTION_AUTH,
                                0U, ML307_SSL_AUTH_SERVER) == ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+MSSLCFG=\"auth\",0,1\r\n") == 0);
  CHECK(ML307_SslBuildSetOption(output, sizeof(output),
                                ML307_SSL_OPTION_VERSION, 5U,
                                ML307_SSL_VERSION_TLS12) == ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+MSSLCFG=\"version\",5,3\r\n") == 0);
  CHECK(ML307_SslBuildSetOption(output, sizeof(output),
                                ML307_SSL_OPTION_IGNORE_TIMESTAMP, 2U, 0U) ==
        ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+MSSLCFG=\"ignorestamp\",2,0\r\n") == 0);
  CHECK(ML307_SslBuildSetOption(output, sizeof(output),
                                ML307_SSL_OPTION_IGNORE_VERIFICATION, 2U, 0U) ==
        ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+MSSLCFG=\"ignoreverify\",2,0\r\n") == 0);
  CHECK(ML307_SslBuildSetOption(output, sizeof(output),
                                ML307_SSL_OPTION_ENCODING, 2U,
                                ML307_SSL_ENCODING_ESCAPED) == ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+MSSLCFG=\"encoding\",2,2\r\n") == 0);

  CHECK(ML307_SslBuildSetOption(output, sizeof(output), ML307_SSL_OPTION_AUTH,
                                6U, 1U) == ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetOption(output, sizeof(output), ML307_SSL_OPTION_AUTH,
                                0U, 0U) == ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetOption(output, sizeof(output),
                                ML307_SSL_OPTION_VERSION, 0U, 255U) ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetOption(output, sizeof(output),
                                ML307_SSL_OPTION_IGNORE_TIMESTAMP, 0U, 1U) ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetOption(output, sizeof(output),
                                ML307_SSL_OPTION_IGNORE_VERIFICATION, 0U, 1U) ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetOption(output, sizeof(output),
                                ML307_SSL_OPTION_ENCODING, 0U, 1U) ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetOption(output, sizeof(output),
                                ML307_SSL_OPTION_CERTIFICATE, 0U, 0U) ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetOption(output, sizeof(output), (ML307_SslOption)99,
                                0U, 0U) == ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetOption(short_output, sizeof(short_output),
                                ML307_SSL_OPTION_AUTH, 0U, 1U) ==
        ML307_RESULT_BUFFER_TOO_SMALL);
  CHECK(short_output[0] == '\0');

  CHECK(ML307_SslBuildQueryOption(output, sizeof(output),
                                  ML307_SSL_OPTION_VERSION, 5U) ==
        ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+MSSLCFG=\"version\",5\r\n") == 0);
  CHECK(ML307_SslBuildQueryOption(output, sizeof(output),
                                  ML307_SSL_OPTION_CERTIFICATE, 0U) ==
        ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+MSSLCFG=\"cert\",0\r\n") == 0);
  CHECK(ML307_SslBuildQueryOption(output, sizeof(output),
                                  ML307_SSL_OPTION_AUTH, 6U) ==
        ML307_RESULT_INVALID_VALUE);
}

static void TestOptionResponses(void)
{
  static const uint8_t valid[] =
      "AT+MSSLCFG=\"auth\",5\r\n+MSSLCFG: \"auth\",5,1\r\nOK\r\n";
  static const uint8_t incomplete[] =
      "+MSSLCFG: \"auth\",5,1\r\nOK";
  static const uint8_t wrong_id[] =
      "+MSSLCFG: \"auth\",4,1\r\nOK\r\n";
  static const uint8_t wrong_key[] =
      "+MSSLCFG: \"version\",5,3\r\nOK\r\n";
  static const uint8_t duplicate[] =
      "+MSSLCFG: \"auth\",5,1\r\n+MSSLCFG: \"auth\",5,1\r\nOK\r\n";
  static const uint8_t inconsistent[] =
      "+MSSLCFG: \"auth\",5,1\r\n+MSSLCFG: \"auth\",5,2\r\nOK\r\n";
  static const uint8_t negative[] = "+CME ERROR: 3\r\n";
  static const uint8_t result_before_data[] =
      "OK\r\n+MSSLCFG: \"auth\",5,1\r\n";
  static const uint8_t invalid_value[] =
      "+MSSLCFG: \"auth\",5,3\r\nOK\r\n";
  static const uint8_t overflow_value[] =
      "+MSSLCFG: \"auth\",5,42949672960\r\nOK\r\n";
  ML307_SslOptionValue value = {ML307_SSL_OPTION_VERSION, 3U, 77U};

  CHECK(ML307_SslParseOptionQuery(valid, sizeof(valid) - 1U,
                                   ML307_SSL_OPTION_AUTH, 5U, &value) ==
        ML307_SSL_PARSE_COMPLETE);
  CHECK(value.option == ML307_SSL_OPTION_AUTH && value.ssl_id == 5U &&
        value.value == 1U);
  value.value = 77U;
  CHECK(ML307_SslParseOptionQuery(incomplete, sizeof(incomplete) - 1U,
                                   ML307_SSL_OPTION_AUTH, 5U, &value) ==
        ML307_SSL_PARSE_INCOMPLETE);
  CHECK(value.value == 77U);
  CHECK(ML307_SslParseOptionQuery(wrong_id, sizeof(wrong_id) - 1U,
                                   ML307_SSL_OPTION_AUTH, 5U, &value) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseOptionQuery(wrong_key, sizeof(wrong_key) - 1U,
                                   ML307_SSL_OPTION_AUTH, 5U, &value) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseOptionQuery(duplicate, sizeof(duplicate) - 1U,
                                   ML307_SSL_OPTION_AUTH, 5U, &value) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseOptionQuery(inconsistent, sizeof(inconsistent) - 1U,
                                   ML307_SSL_OPTION_AUTH, 5U, &value) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseOptionQuery(negative, sizeof(negative) - 1U,
                                   ML307_SSL_OPTION_AUTH, 5U, &value) ==
        ML307_SSL_PARSE_ERROR_RESPONSE);
  CHECK(ML307_SslParseOptionQuery(result_before_data,
                                   sizeof(result_before_data) - 1U,
                                   ML307_SSL_OPTION_AUTH, 5U, &value) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseOptionQuery(invalid_value, sizeof(invalid_value) - 1U,
                                   ML307_SSL_OPTION_AUTH, 5U, &value) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseOptionQuery(overflow_value, sizeof(overflow_value) - 1U,
                                   ML307_SSL_OPTION_AUTH, 5U, &value) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseOptionQuery(NULL, 0U, ML307_SSL_OPTION_AUTH, 0U,
                                   &value) == ML307_SSL_PARSE_INVALID);
}

static void TestCertificateNames(void)
{
  char output[128];
  char name64[65];
  char name65[66];
  static const uint8_t valid[] =
      "+MSSLCFG: \"cert\",2,\"root.pem\",\"\",\"\"\r\nOK\r\n";
  static const uint8_t populated[] =
      "+MSSLCFG: \"cert\",2,\"root.pem\",\"client.pem\",\"key.pem\"\r\n"
      "OK\r\n";
  static const uint8_t wrong_id[] =
      "+MSSLCFG: \"cert\",1,\"root.pem\",\"\",\"\"\r\nOK\r\n";
  static const uint8_t wrong_key[] =
      "+MSSLCFG: \"auth\",2,1\r\nOK\r\n";
  static const uint8_t embedded_nul[] =
      "+MSSLCFG: \"cert\",2,\"expected-name\0other\",\"\",\"\"\r\nOK\r\n";
  ML307_SslCertificateNames names = {{0}, {0}, {0}};

  memset(name64, 'a', 64U);
  name64[64] = '\0';
  memset(name65, 'b', 65U);
  name65[65] = '\0';
  CHECK(ML307_SslBuildSetCa(output, sizeof(output), 5U, name64) ==
        ML307_RESULT_OK);
  CHECK(strlen(output) == strlen("AT+MSSLCFG=\"cert\",5,\"\",\"\",\"\"\r\n") +
                              64U);
  CHECK(ML307_SslBuildSetCa(output, sizeof(output), 6U, "root.pem") ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetCa(output, sizeof(output), 0U, "") ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetCa(output, sizeof(output), 0U, name65) ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetCa(output, sizeof(output), 0U, "a,b") ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetCa(output, sizeof(output), 0U, "a\"b") ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetCa(output, sizeof(output), 0U, "a\rb") ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildSetCa(output, sizeof(output), 0U, "a\nb") ==
        ML307_RESULT_INVALID_VALUE);

  CHECK(ML307_SslParseCertificateQuery(valid, sizeof(valid) - 1U, 2U,
                                        &names) == ML307_SSL_PARSE_COMPLETE);
  CHECK(strcmp(names.server_ca, "root.pem") == 0);
  CHECK(names.client_certificate[0] == '\0' && names.private_key[0] == '\0');
  CHECK(ML307_SslParseCertificateQuery(populated, sizeof(populated) - 1U, 2U,
                                        &names) == ML307_SSL_PARSE_COMPLETE);
  CHECK(strcmp(names.client_certificate, "client.pem") == 0);
  CHECK(strcmp(names.private_key, "key.pem") == 0);
  strcpy(names.server_ca, "unchanged");
  CHECK(ML307_SslParseCertificateQuery(wrong_id, sizeof(wrong_id) - 1U, 2U,
                                        &names) == ML307_SSL_PARSE_INVALID);
  CHECK(strcmp(names.server_ca, "unchanged") == 0);
  CHECK(ML307_SslParseCertificateQuery(wrong_key, sizeof(wrong_key) - 1U, 2U,
                                        &names) == ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseCertificateQuery(embedded_nul,
                                        sizeof(embedded_nul) - 1U, 2U,
                                        &names) == ML307_SSL_PARSE_INVALID);
  CHECK(strcmp(names.server_ca, "unchanged") == 0);
}

static void TestCertificateWrite(void)
{
  static uint8_t certificate[ML307_SSL_CERTIFICATE_MAX];
  char output[128];
  const uint8_t *payload = NULL;
  size_t payload_length = 0U;
  ML307_SslCertificateChunk chunk = {
      "root.pem", certificate, sizeof(certificate), 0U, 1024U};
  static const uint8_t prompt[] = "AT+MSSLCERTWR=\"root.pem\",0,3\r\n\r\n> ";
  static const uint8_t echo_with_gt[] =
      "AT+MSSLCERTWR=\"a>b.pem\",0,3\r\n";
  static const uint8_t error[] = "+CME ERROR: 50\r\n";
  static const uint8_t error_then_prompt[] = "ERROR\r\n>";
  static const uint8_t cme_then_prompt[] = "+CME ERROR: 50\r\n> ";
  static const uint8_t ok_then_prompt[] = "OK\r\n>";
  static const uint8_t incomplete_error[] = "+CME ERROR: 50";

  CHECK(ML307_SslBuildCertificateWrite(output, sizeof(output), &chunk) ==
        ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+MSSLCERTWR=\"root.pem\",7168,1024\r\n") == 0);
  CHECK(ML307_SslGetCertificateWritePayload(&chunk, &payload,
                                             &payload_length) ==
        ML307_RESULT_OK);
  CHECK(payload == certificate && payload_length == 1024U);
  chunk.offset = 7168U;
  CHECK(ML307_SslBuildCertificateWrite(output, sizeof(output), &chunk) ==
        ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+MSSLCERTWR=\"root.pem\",0,1024\r\n") == 0);
  CHECK(ML307_SslGetCertificateWritePayload(&chunk, &payload,
                                             &payload_length) ==
        ML307_RESULT_OK);
  CHECK(payload == certificate + 7168U && payload_length == 1024U);

  chunk.certificate_length = ML307_SSL_CERTIFICATE_MAX + 1U;
  CHECK(ML307_SslBuildCertificateWrite(output, sizeof(output), &chunk) ==
        ML307_RESULT_INVALID_VALUE);
  chunk.certificate_length = sizeof(certificate);
  chunk.offset = sizeof(certificate);
  CHECK(ML307_SslBuildCertificateWrite(output, sizeof(output), &chunk) ==
        ML307_RESULT_INVALID_VALUE);
  chunk.offset = 8000U;
  chunk.length = 193U;
  CHECK(ML307_SslBuildCertificateWrite(output, sizeof(output), &chunk) ==
        ML307_RESULT_INVALID_VALUE);
  chunk.offset = 0U;
  chunk.length = 0U;
  CHECK(ML307_SslBuildCertificateWrite(output, sizeof(output), &chunk) ==
        ML307_RESULT_INVALID_VALUE);
  chunk.length = 1U;
  chunk.name = "bad,name";
  CHECK(ML307_SslBuildCertificateWrite(output, sizeof(output), &chunk) ==
        ML307_RESULT_INVALID_VALUE);

  CHECK(ML307_SslParseCertificateWritePrompt(prompt, sizeof(prompt) - 1U) ==
        ML307_SSL_PARSE_COMPLETE);
  CHECK(ML307_SslParseCertificateWritePrompt(prompt, sizeof(prompt) - 3U) ==
        ML307_SSL_PARSE_INCOMPLETE);
  CHECK(ML307_SslParseCertificateWritePrompt(echo_with_gt,
                                              sizeof(echo_with_gt) - 1U) ==
        ML307_SSL_PARSE_INCOMPLETE);
  CHECK(ML307_SslParseCertificateWritePrompt(error, sizeof(error) - 1U) ==
        ML307_SSL_PARSE_ERROR_RESPONSE);
  CHECK(ML307_SslParseCertificateWritePrompt(error_then_prompt,
                                              sizeof(error_then_prompt) - 1U) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseCertificateWritePrompt(cme_then_prompt,
                                              sizeof(cme_then_prompt) - 1U) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseCertificateWritePrompt(ok_then_prompt,
                                              sizeof(ok_then_prompt) - 1U) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseCertificateWritePrompt(incomplete_error,
                                              sizeof(incomplete_error) - 1U) ==
        ML307_SSL_PARSE_INCOMPLETE);
  CHECK(ML307_SslParseCertificateWritePrompt((const uint8_t *)">x", 2U) ==
        ML307_SSL_PARSE_INVALID);

  CHECK(ML307_SslParseFinalResult((const uint8_t *)"\r\nOK\r\n", 6U) ==
        ML307_SSL_PARSE_COMPLETE);
  CHECK(ML307_SslParseFinalResult((const uint8_t *)"OK", 2U) ==
        ML307_SSL_PARSE_INCOMPLETE);
  CHECK(ML307_SslParseFinalResult((const uint8_t *)"ERROR\r\n", 7U) ==
        ML307_SSL_PARSE_ERROR_RESPONSE);
  CHECK(ML307_SslParseFinalResult((const uint8_t *)"OK\r\nOK\r\n", 8U) ==
        ML307_SSL_PARSE_INVALID);
}

static size_t BuildCertificateReadFrame(uint8_t *frame, size_t capacity,
                                        const uint8_t *payload,
                                        size_t payload_length)
{
  int header_length = snprintf((char *)frame, capacity,
                               "AT+MSSLCERTRD=\"root.pem\"\r\n"
                               "+MSSLCERTRD: %lu,",
                               (unsigned long)payload_length);
  size_t used;
  static const uint8_t tail[] = "\r\nOK\r\n";
  if (header_length < 0) return 0U;
  used = (size_t)header_length;
  if (used + payload_length + sizeof(tail) - 1U > capacity) return 0U;
  memcpy(frame + used, payload, payload_length);
  used += payload_length;
  memcpy(frame + used, tail, sizeof(tail) - 1U);
  return used + sizeof(tail) - 1U;
}

static void TestCertificateRead(void)
{
  static const uint8_t fake_control_payload[] = {
      'P', 'E', 'M', '\r', '\n', 'O', 'K', '\r', '\n', 'E', 'R', 'R', 'O', 'R',
      '\r', '\n', '+', 'M', 'A', 'T', 'R', 'E', 'A', 'D', 'Y', '\r', '\n',
      '+', 'M', 'Q', 'T', 'T', 'U', 'R', 'C', ':', 'x', '\0', 'Z'};
  uint8_t frame[512];
  uint8_t decoded[64];
  uint8_t before[64];
  uint8_t duplicate[1024];
  uint8_t conflicting[1024];
  size_t decoded_length = 123U;
  size_t frame_length;
  size_t cut;
  char command[96];
  static const uint8_t too_large[] = "+MSSLCERTRD: 8193,x\r\nOK\r\n";
  static const uint8_t overflow[] =
      "+MSSLCERTRD: 42949672960,x\r\nOK\r\n";
  static const uint8_t negative[] = "+CME ERROR: 50\r\n";
  static const uint8_t error_prefix[] = "ERROR\r\n";
  static const uint8_t ok_prefix[] = "OK\r\n";
  static const uint8_t module_ready[] = "+MATREADY\r\n";

  CHECK(ML307_SslBuildCertificateRead(command, sizeof(command), "root.pem") ==
        ML307_RESULT_OK);
  CHECK(strcmp(command, "AT+MSSLCERTRD=\"root.pem\"\r\n") == 0);
  CHECK(ML307_SslBuildCertificateRead(command, sizeof(command), "a,b") ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildCertificateRead(command, sizeof(command), "a\"b") ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildCertificateRead(command, sizeof(command), "") ==
        ML307_RESULT_INVALID_VALUE);

  frame_length = BuildCertificateReadFrame(
      frame, sizeof(frame), fake_control_payload, sizeof(fake_control_payload));
  CHECK(frame_length > 0U);
  memset(decoded, 0xA5, sizeof(decoded));
  memcpy(before, decoded, sizeof(decoded));
  for (cut = 0U; cut < frame_length; ++cut) {
    decoded_length = 123U;
    CHECK(ML307_SslParseCertificateRead(frame, cut, decoded, sizeof(decoded),
                                         &decoded_length) ==
          ML307_SSL_PARSE_INCOMPLETE);
    CHECK(decoded_length == 123U);
    CHECK(memcmp(decoded, before, sizeof(decoded)) == 0);
  }
  CHECK(ML307_SslParseCertificateRead(frame, frame_length, decoded,
                                       sizeof(decoded), &decoded_length) ==
        ML307_SSL_PARSE_COMPLETE);
  CHECK(decoded_length == sizeof(fake_control_payload));
  CHECK(memcmp(decoded, fake_control_payload, decoded_length) == 0);
  memset(decoded, 0x5A, sizeof(decoded));
  memcpy(before, decoded, sizeof(decoded));
  decoded_length = 456U;
  memcpy(conflicting, error_prefix, sizeof(error_prefix) - 1U);
  memcpy(conflicting + sizeof(error_prefix) - 1U, frame, frame_length);
  CHECK(ML307_SslParseCertificateRead(
            conflicting, sizeof(error_prefix) - 1U + frame_length, decoded,
            sizeof(decoded), &decoded_length) == ML307_SSL_PARSE_INVALID);
  CHECK(decoded_length == 456U);
  CHECK(memcmp(decoded, before, sizeof(decoded)) == 0);
  memcpy(conflicting, ok_prefix, sizeof(ok_prefix) - 1U);
  memcpy(conflicting + sizeof(ok_prefix) - 1U, frame, frame_length);
  CHECK(ML307_SslParseCertificateRead(
            conflicting, sizeof(ok_prefix) - 1U + frame_length, decoded,
            sizeof(decoded), &decoded_length) == ML307_SSL_PARSE_INVALID);
  CHECK(decoded_length == 456U);
  CHECK(memcmp(decoded, before, sizeof(decoded)) == 0);
  memcpy(duplicate, frame, frame_length);
  memcpy(duplicate + frame_length, frame, frame_length);
  CHECK(ML307_SslParseCertificateRead(duplicate, frame_length * 2U, decoded,
                                       sizeof(decoded), &decoded_length) ==
        ML307_SSL_PARSE_INVALID);

  memset(decoded, 0x3C, sizeof(decoded));
  memcpy(before, decoded, sizeof(decoded));
  decoded_length = 654U;
  memcpy(conflicting, module_ready, sizeof(module_ready) - 1U);
  memcpy(conflicting + sizeof(module_ready) - 1U, frame, frame_length);
  CHECK(ML307_SslParseCertificateRead(
            conflicting, sizeof(module_ready) - 1U + frame_length, decoded,
            sizeof(decoded), &decoded_length) == ML307_SSL_PARSE_MODULE_RESET);
  CHECK(decoded_length == 654U);
  CHECK(memcmp(decoded, before, sizeof(decoded)) == 0);
  memcpy(conflicting, frame, frame_length);
  memcpy(conflicting + frame_length, module_ready, sizeof(module_ready) - 1U);
  CHECK(ML307_SslParseCertificateRead(
            conflicting, frame_length + sizeof(module_ready) - 1U, decoded,
            sizeof(decoded), &decoded_length) == ML307_SSL_PARSE_MODULE_RESET);
  CHECK(decoded_length == 654U);
  CHECK(memcmp(decoded, before, sizeof(decoded)) == 0);

  decoded_length = 321U;
  CHECK(ML307_SslParseCertificateRead(frame, frame_length, decoded, 4U,
                                       &decoded_length) ==
        ML307_SSL_PARSE_BUFFER_TOO_SMALL);
  CHECK(decoded_length == 321U);
  frame[frame_length++] = '\r';
  frame[frame_length++] = '\n';
  CHECK(ML307_SslParseCertificateRead(frame, frame_length, decoded,
                                       sizeof(decoded), &decoded_length) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseCertificateRead(too_large, sizeof(too_large) - 1U,
                                       decoded, sizeof(decoded),
                                       &decoded_length) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseCertificateRead(overflow, sizeof(overflow) - 1U, decoded,
                                       sizeof(decoded), &decoded_length) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseCertificateRead(negative, sizeof(negative) - 1U, decoded,
                                       sizeof(decoded), &decoded_length) ==
        ML307_SSL_PARSE_ERROR_RESPONSE);
}

static void TestClock(void)
{
  char output[64];
  ML307_ModuleClock clock = {1970U, 1U, 1U, 0U, 0U, 0U, 0};
  ML307_ModuleClock parsed = {2001U, 2U, 3U, 4U, 5U, 6U, 8};
  static const uint8_t valid[] =
      "AT+CCLK?\r\n+CCLK: \"37/12/31,23:59:59+56\"\r\nOK\r\n";
  static const uint8_t utc[] = "+CCLK: \"70/01/01,00:00:00+00\"\r\nOK\r\n";
  static const uint8_t lower_zone[] =
      "+CCLK: \"20/01/01,00:00:00-48\"\r\nOK\r\n";
  static const uint8_t forbidden_zone[] =
      "+CCLK: \"20/01/01,00:00:00-01\"\r\nOK\r\n";
  static const uint8_t forbidden_positive_zone[] =
      "+CCLK: \"20/01/01,00:00:00+01\"\r\nOK\r\n";
  static const uint8_t below_zone[] =
      "+CCLK: \"20/01/01,00:00:00-49\"\r\nOK\r\n";
  static const uint8_t above_zone[] =
      "+CCLK: \"20/01/01,00:00:00+57\"\r\nOK\r\n";
  static const uint8_t out_of_range_year[] =
      "+CCLK: \"38/01/01,00:00:00+00\"\r\nOK\r\n";
  static const uint8_t incomplete[] =
      "+CCLK: \"20/01/01,00:00:00+00\"\r\nOK";
  static const uint8_t negative[] = "+CME ERROR: 3\r\n";
  static const uint8_t duplicate[] =
      "+CCLK: \"20/01/01,00:00:00+00\"\r\n"
      "+CCLK: \"20/01/01,00:00:00+00\"\r\nOK\r\n";

  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+CCLK=\"70/01/01,00:00:00+00\"\r\n") == 0);
  clock.year = 2037U;
  clock.month = 12U;
  clock.day = 31U;
  clock.hour = 23U;
  clock.minute = 59U;
  clock.second = 59U;
  clock.timezone_quarters = 56;
  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+CCLK=\"37/12/31,23:59:59+56\"\r\n") == 0);
  clock.timezone_quarters = -48;
  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_OK);
  clock.timezone_quarters = 57;
  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_INVALID_VALUE);
  clock.timezone_quarters = -49;
  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_INVALID_VALUE);
  clock.timezone_quarters = -1;
  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_INVALID_VALUE);
  clock.timezone_quarters = 1;
  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_INVALID_VALUE);
  clock.timezone_quarters = 0;
  clock.year = 1969U;
  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_INVALID_VALUE);
  clock.year = 2038U;
  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_INVALID_VALUE);
  clock.year = 2001U;
  clock.month = 2U;
  clock.day = 29U;
  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_INVALID_VALUE);
  clock.year = 2000U;
  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_OK);
  clock.hour = 24U;
  CHECK(ML307_SslBuildClockSet(output, sizeof(output), &clock) ==
        ML307_RESULT_INVALID_VALUE);
  CHECK(ML307_SslBuildClockQuery(output, sizeof(output)) == ML307_RESULT_OK);
  CHECK(strcmp(output, "AT+CCLK?\r\n") == 0);

  CHECK(ML307_SslParseClockQuery(valid, sizeof(valid) - 1U, &parsed) ==
        ML307_SSL_PARSE_COMPLETE);
  CHECK(parsed.year == 2037U && parsed.month == 12U && parsed.day == 31U &&
        parsed.hour == 23U && parsed.minute == 59U && parsed.second == 59U &&
        parsed.timezone_quarters == 56);
  CHECK(ML307_SslParseClockQuery(utc, sizeof(utc) - 1U, &parsed) ==
        ML307_SSL_PARSE_COMPLETE);
  CHECK(parsed.year == 1970U && parsed.timezone_quarters == 0);
  CHECK(ML307_SslParseClockQuery(lower_zone, sizeof(lower_zone) - 1U,
                                  &parsed) == ML307_SSL_PARSE_COMPLETE);
  CHECK(parsed.timezone_quarters == -48);
  parsed.year = 1999U;
  CHECK(ML307_SslParseClockQuery(forbidden_zone,
                                  sizeof(forbidden_zone) - 1U, &parsed) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(parsed.year == 1999U);
  CHECK(ML307_SslParseClockQuery(forbidden_positive_zone,
                                  sizeof(forbidden_positive_zone) - 1U,
                                  &parsed) == ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseClockQuery(below_zone, sizeof(below_zone) - 1U,
                                  &parsed) == ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseClockQuery(above_zone, sizeof(above_zone) - 1U,
                                  &parsed) == ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseClockQuery(out_of_range_year,
                                  sizeof(out_of_range_year) - 1U, &parsed) ==
        ML307_SSL_PARSE_INVALID);
  CHECK(ML307_SslParseClockQuery(incomplete, sizeof(incomplete) - 1U,
                                  &parsed) == ML307_SSL_PARSE_INCOMPLETE);
  CHECK(ML307_SslParseClockQuery(negative, sizeof(negative) - 1U, &parsed) ==
        ML307_SSL_PARSE_ERROR_RESPONSE);
  CHECK(ML307_SslParseClockQuery(duplicate, sizeof(duplicate) - 1U, &parsed) ==
        ML307_SSL_PARSE_INVALID);
}

static void TestTransactions(void)
{
  static const uint8_t certificate[] =
      "-----BEGIN CERTIFICATE-----\nQUJD\n-----END CERTIFICATE-----\n";
  uint8_t frame[256];
  uint8_t scratch[sizeof(certificate)];
  char command[128];
  ML307_SslAction action;
  ML307_SslTransaction transaction;
  ML307_ModuleClock clock;
  size_t frame_length;
  static const uint8_t prompt[] = "\r\n> ";
  static const uint8_t ok[] = "OK\r\n";

  ML307_SslTransactionInit(&transaction);
  CHECK(ML307_SslTransactionStartCaInstall(
            &transaction, "mjy-ca-00000001.pem", certificate,
            sizeof(certificate) - 1U) == ML307_RESULT_OK);
  CHECK(ML307_SslTransactionGetAction(&transaction, command, sizeof(command),
                                       &action) == ML307_RESULT_OK);
  CHECK(action.kind == ML307_SSL_ACTION_AT_COMMAND &&
        !strcmp(command,
                "AT+MSSLCERTWR=\"mjy-ca-00000001.pem\",0,59\r\n"));
  CHECK(ML307_SslTransactionActionSent(&transaction) == ML307_RESULT_OK);
  CHECK(ML307_SslTransactionConsumeResponse(&transaction, prompt,
                                             sizeof(prompt) - 1U, scratch,
                                             sizeof(scratch)) ==
        ML307_SSL_PARSE_COMPLETE);
  CHECK(ML307_SslTransactionGetAction(&transaction, command, sizeof(command),
                                       &action) == ML307_RESULT_OK);
  CHECK(action.kind == ML307_SSL_ACTION_RAW_PAYLOAD &&
        action.data == certificate && action.length == sizeof(certificate) - 1U);
  CHECK(ML307_SslTransactionActionSent(&transaction) == ML307_RESULT_OK);
  CHECK(ML307_SslTransactionConsumeResponse(&transaction, ok,
                                             sizeof(ok) - 1U, scratch,
                                             sizeof(scratch)) ==
        ML307_SSL_PARSE_COMPLETE);
  CHECK(ML307_SslTransactionGetAction(&transaction, command, sizeof(command),
                                       &action) == ML307_RESULT_OK);
  CHECK(!strcmp(command, "AT+MSSLCERTRD=\"mjy-ca-00000001.pem\"\r\n"));
  CHECK(ML307_SslTransactionActionSent(&transaction) == ML307_RESULT_OK);
  frame_length = BuildCertificateReadFrame(frame, sizeof(frame), certificate,
                                            sizeof(certificate) - 1U);
  CHECK(ML307_SslTransactionConsumeResponse(
            &transaction, frame, frame_length, scratch, sizeof(scratch)) ==
        ML307_SSL_PARSE_COMPLETE);
  CHECK(ML307_SslTransactionGetStatus(&transaction) ==
        ML307_SSL_TRANSACTION_COMPLETE);

  ML307_SslTransactionInit(&transaction);
  CHECK(ML307_SslClockFromUnixUtc(1780000000U, &clock) == ML307_RESULT_OK);
  CHECK(clock.year >= 2026U && clock.year <= 2037U &&
        clock.timezone_quarters == 0);
  CHECK(ML307_SslTransactionStartPrepare(
            &transaction, 3U, 0U, "mjy-ca-00000001.pem", &clock) ==
        ML307_RESULT_OK);
  while (ML307_SslTransactionGetStatus(&transaction) ==
         ML307_SSL_TRANSACTION_ACTIVE) {
    uint8_t response[192];
    size_t response_length;
    CHECK(ML307_SslTransactionGetAction(&transaction, command,
                                         sizeof(command), &action) ==
          ML307_RESULT_OK);
    CHECK(action.kind == ML307_SSL_ACTION_AT_COMMAND);
    CHECK(strstr(command, "MQTTCONN") == NULL);
    CHECK(ML307_SslTransactionActionSent(&transaction) == ML307_RESULT_OK);
    if (strstr(command, "AT+MSSLCFG=\"encoding\",3\r\n") != NULL)
      response_length = (size_t)snprintf((char *)response, sizeof(response),
          "+MSSLCFG: \"encoding\",3,2\r\nOK\r\n");
    else if (strstr(command, "AT+MSSLCFG=\"auth\",3\r\n") != NULL)
      response_length = (size_t)snprintf((char *)response, sizeof(response),
          "+MSSLCFG: \"auth\",3,1\r\nOK\r\n");
    else if (strstr(command, "AT+MSSLCFG=\"version\",3\r\n") != NULL)
      response_length = (size_t)snprintf((char *)response, sizeof(response),
          "+MSSLCFG: \"version\",3,3\r\nOK\r\n");
    else if (strstr(command, "AT+MSSLCFG=\"ignorestamp\",3\r\n") != NULL)
      response_length = (size_t)snprintf((char *)response, sizeof(response),
          "+MSSLCFG: \"ignorestamp\",3,0\r\nOK\r\n");
    else if (strstr(command, "AT+MSSLCFG=\"ignoreverify\",3\r\n") != NULL)
      response_length = (size_t)snprintf((char *)response, sizeof(response),
          "+MSSLCFG: \"ignoreverify\",3,0\r\nOK\r\n");
    else if (strstr(command, "AT+MSSLCFG=\"cert\",3\r\n") != NULL)
      response_length = (size_t)snprintf((char *)response, sizeof(response),
          "+MSSLCFG: \"cert\",3,\"mjy-ca-00000001.pem\",\"\",\"\"\r\nOK\r\n");
    else if (!strcmp(command, "AT+CCLK?\r\n"))
      response_length = (size_t)snprintf((char *)response, sizeof(response),
          "+CCLK: \"%02u/%02u/%02u,%02u:%02u:%02u+00\"\r\nOK\r\n",
          clock.year % 100U, clock.month, clock.day, clock.hour, clock.minute,
          (unsigned int)(clock.second + (clock.second < 58U ? 2U : 0U)));
    else if (!strcmp(command, "AT+MQTTCFG=\"ssl\",0\r\n"))
      response_length = (size_t)snprintf((char *)response, sizeof(response),
          "AT+MQTTCFG=\"ssl\",0\r\n+MQTTCFG: \"ssl\",1,3\r\nOK\r\n");
    else {
      memcpy(response, ok, sizeof(ok) - 1U);
      response_length = sizeof(ok) - 1U;
    }
    CHECK(ML307_SslTransactionConsumeResponse(
              &transaction, response, response_length, scratch,
              sizeof(scratch)) == ML307_SSL_PARSE_COMPLETE);
  }
  CHECK(ML307_SslTransactionGetStatus(&transaction) ==
        ML307_SSL_TRANSACTION_COMPLETE);
}

int main(void)
{
  TestOptionCommands();
  TestOptionResponses();
  TestCertificateNames();
  TestCertificateWrite();
  TestCertificateRead();
  TestClock();
  TestTransactions();
  if (failures != 0) {
    fprintf(stderr, "ml307_ssl_test: %d failure(s)\n", failures);
    return 1;
  }
  puts("ml307_ssl_test: PASS");
  return 0;
}
