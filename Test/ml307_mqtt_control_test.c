#include <ML307/ml307_mqtt.h>
#include <AT/ModuleFrameParser.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTES(s) (const uint8_t *)(s), strlen(s)
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)

static void expect_control(const char *text, ML307_MqttEventType type,
                            uint8_t cid, uint16_t mid, int state)
{
  ML307_MqttEvent event, legacy;
  CHECK(ML307_MqttParseControlUrc(BYTES(text), &event) == ML307_RESULT_OK);
  CHECK(event.type == type && event.connect_id == cid &&
        event.message_id == mid && event.state == state);
  CHECK(ML307_MqttParseUrc(text, &legacy) == ML307_RESULT_OK);
  CHECK(legacy.type == event.type && legacy.connect_id == event.connect_id &&
        legacy.message_id == event.message_id && legacy.state == event.state);
}

static void test_valid_controls(void)
{
  unsigned cid, code;
  char text[96];
  for (cid = 0; cid <= 5U; ++cid) {
    for (code = 0; code <= 6U; ++code) {
      snprintf(text, sizeof(text), "\t+MQTTURC: \"conn\", %u, %u \r\n", cid, code);
      expect_control(text, ML307_MQTT_EVENT_CONNECTION, (uint8_t)cid, 0U, (int)code);
    }
    snprintf(text, sizeof(text), "+MQTTURC: \"conn\",%u,255", cid);
    expect_control(text, ML307_MQTT_EVENT_CONNECTION, (uint8_t)cid, 0U, 255);
    for (code = 0; code <= 2U; ++code) {
      ML307_MqttEvent event;
      snprintf(text, sizeof(text), "+MQTTURC: \"suback\",%u,65535,%u", cid, code);
      expect_control(text, ML307_MQTT_EVENT_SUBACK, (uint8_t)cid, UINT16_MAX, (int)code);
      CHECK(ML307_MqttParseControlUrc(BYTES(text), &event) == ML307_RESULT_OK);
      CHECK(event.qos == code);
    }
    snprintf(text, sizeof(text), "+MQTTURC: \"suback\",%u,42,128", cid);
    expect_control(text, ML307_MQTT_EVENT_SUBACK, (uint8_t)cid, 42U, 128);
    for (code = 0; code <= 1U; ++code) {
      snprintf(text, sizeof(text), "+MQTTURC: \"puback\",%u,42,%u", cid, code);
      expect_control(text, ML307_MQTT_EVENT_PUBACK, (uint8_t)cid, 42U, (int)code);
    }
    snprintf(text, sizeof(text), "+MQTTURC: \"timeout\",%u,0", cid);
    expect_control(text, ML307_MQTT_EVENT_TIMEOUT, (uint8_t)cid, 0U, 0);
  }
}

static void test_malformed_controls(void)
{
  static const char *const bad[] = {
    "+MQTTURC: \"suback\",0,42", "+MQTTURC: \"suback\",0,42,",
    "+MQTTURC: \"suback\",0,42,1junk", "+MQTTURC: \"suback\",0,42,1,0",
    "+MQTTURC: \"suback\",0,42,-1", "+MQTTURC: \"suback\",0,42,+1",
    "+MQTTURC: \"suback\",0,42,3", "+MQTTURC: \"suback\",0,42,129",
    "+MQTTURC: \"suback\",0,42,4294967296", "+MQTTURC: \"suback\",0,65578,1",
    "+MQTTURC: \"suback\",256,42,1", "+MQTTURC: \"suback\",6,42,1",
    "+MQTTURC: \"timeout\",0", "+MQTTURC: \"timeout\",0,42,1",
    "+MQTTURC: \"puback\",0,42", "+MQTTURC: \"puback\",0,42,2",
    "+MQTTURC: \"puback\",0,42,0extra", "+MQTTURC: \"puback\",-1,42,0",
    "+MQTTURC: \"conn\",0", "+MQTTURC: \"conn\",0,7",
    "+MQTTURC: \"conn\",0,256", "+MQTTURC: \"conn\",0,0junk",
    "+MQTTURC: \"conn\",0,0,1", "+MQTTURC: \"conn\",0,0\rjunk",
    "+MQTTURC: \"suback\"", "+MQTTURC: \"suback\",0,,1",
    "+MQTTURC: \"suback\",0,42,9999999999999999999999999999999999",
    "noise +MQTTURC: \"suback\",0,42,0", "OK +MQTTURC: \"conn\",0,0"
  };
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
    ML307_MqttEvent event;
    memset(&event, 0xAA, sizeof(event));
    CHECK(ML307_MqttParseControlUrc(BYTES(bad[i]), &event) != ML307_RESULT_OK);
    CHECK(event.type == ML307_MQTT_EVENT_NONE && event.state == 0);
    CHECK(ML307_MqttParseUrc(bad[i], &event) != ML307_RESULT_OK);
    CHECK(event.type == ML307_MQTT_EVENT_NONE);
  }
  {
    const char text[] = "+MQTTURC: \"suback\",0,42,0\0junk";
    ML307_MqttEvent event;
    CHECK(ML307_MqttParseControlUrc((const uint8_t *)text, sizeof(text) - 1U, &event) ==
          ML307_RESULT_INVALID_VALUE);
    CHECK(ML307_MqttParseControlUrc(NULL, 1U, &event) == ML307_RESULT_INVALID_ARGUMENT);
    CHECK(event.type == ML307_MQTT_EVENT_NONE);
    CHECK(ML307_MqttParseControlUrc(BYTES("x"), NULL) == ML307_RESULT_INVALID_ARGUMENT);
  }
}

static void test_command_responses(void)
{
  uint8_t cid = 9U;
  uint16_t mid = 999U;
  uint32_t state = 99U;
  const char *sub = "\r\nAT+MQTTSUB=0,\"a\",1\r\n+MQTTSUB: 0,35270\r\nOK\r\n";
  CHECK(ML307_MqttParseSubResponse(BYTES(sub), &cid, &mid) == ML307_RESULT_OK);
  CHECK(cid == 0U && mid == 35270U);
  CHECK(ML307_MqttParsePubResponse(BYTES("+MQTTPUB: 5,65535,4294967295\r\n"), &cid, &mid) == ML307_RESULT_OK);
  CHECK(cid == 5U && mid == UINT16_MAX);
  const char *const bad_sub[] = {"+MQTTSUB: 0,42", "+MQTTSUB: 0,42\r",
    "+MQTTSUB: 0,42,0\n", "+MQTTSUB: 0,42junk\n", "+MQTTSUB: 0,65536\n",
    "+MQTTSUB: 6,42\n", "junk+MQTTSUB: 0,42\n", "OK\r\n",
    "+MQTTSUB: 0,42junk\n+MQTTSUB: 0,42\n"};
  for (size_t i = 0; i < sizeof(bad_sub) / sizeof(bad_sub[0]); ++i) {
    cid = 9U; mid = 999U;
    CHECK(ML307_MqttParseSubResponse(BYTES(bad_sub[i]), &cid, &mid) != ML307_RESULT_OK);
    CHECK(cid == 9U && mid == 999U);
  }
  CHECK(ML307_MqttParsePubResponse(BYTES("+MQTTPUB: 0,42\n"), &cid, &mid) != ML307_RESULT_OK);
  CHECK(ML307_MqttParsePubResponse(BYTES("+MQTTPUB: 0,42,-1\n"), &cid, &mid) != ML307_RESULT_OK);
  CHECK(ML307_MqttParsePubResponse(BYTES("+MQTTPUB: 0,42,4294967296\n"), &cid, &mid) != ML307_RESULT_OK);
  CHECK(ML307_MqttParseStateResponse(BYTES("+MQTTSTATE: 3\r\nOK\r\n"), &state) == ML307_RESULT_OK);
  CHECK(state == 3U);
  CHECK(ML307_MqttParseStateResponse(BYTES("+MQTTSTATE: 3,1\r\n"), &state) != ML307_RESULT_OK);
  CHECK(state == 3U);
  CHECK(ML307_MqttParseStateResponse(BYTES("+MQTTSTATE: 4294967295\n"), &state) == ML307_RESULT_OK);
  CHECK(state == UINT32_MAX);
  CHECK(ML307_MqttParseSubResponse(BYTES(sub), NULL, &mid) == ML307_RESULT_INVALID_ARGUMENT);
  CHECK(ML307_MqttParseSubResponse(NULL, 10U, &cid, &mid) == ML307_RESULT_INVALID_ARGUMENT);
  CHECK(ML307_MqttParsePubResponse(BYTES(sub), &cid, NULL) == ML307_RESULT_INVALID_ARGUMENT);
  CHECK(ML307_MqttParseStateResponse(BYTES(sub), NULL) == ML307_RESULT_INVALID_ARGUMENT);
}

static void test_coalesced_connection_responses(void)
{
  const char *response = "+MQTTURC: \"conn\",0,1\r\n+MQTTURC: \"conn\",1,6\r\n"
                         "+MQTTURC: \"conn\",0,0\r\n+MQTTURC: \"conn\",0,3";
  uint32_t state = 9U;
  CHECK(ML307_MqttParseConnectionResponse(BYTES(response), 0U, &state) == ML307_RESULT_OK);
  CHECK(state == 0U); /* Last complete valid line; not the partial trailing line. */
  CHECK(ML307_MqttParseConnectionResponse(BYTES(response), 1U, &state) == ML307_RESULT_OK);
  CHECK(state == 6U);
  CHECK(ML307_MqttParseConnectionResponse(BYTES(response), 2U, &state) == ML307_RESULT_NOT_FOUND);
  CHECK(state == 6U);
  CHECK(ML307_MqttParseConnectionResponse(BYTES("+MQTTURC: \"conn\",0,0\r"), 0U, &state) != ML307_RESULT_OK);
  CHECK(ML307_MqttParseConnectionResponse(BYTES(response), 6U, &state) == ML307_RESULT_INVALID_ARGUMENT);
  CHECK(ML307_MqttParseConnectionResponse(NULL, 0U, 0U, &state) == ML307_RESULT_INVALID_ARGUMENT);
  CHECK(ML307_MqttParseConnectionResponse(BYTES(response), 0U, NULL) == ML307_RESULT_INVALID_ARGUMENT);
}

static void test_bounded_slices_and_noise(void)
{
  /* Exact allocations with no NUL: ASan redzones detect reads past length. */
  const char *seeds[] = {"+MQTTURC: \"suback\",0,42,1\r\n", "+MQTTSUB: 0,42\r\n",
                        "+MQTTPUB: 0,42,10\r\n", "+MQTTURC: \"conn\",0,255\r\n"};
  for (size_t s = 0; s < sizeof(seeds) / sizeof(seeds[0]); ++s) {
    for (size_t n = 1U; n <= strlen(seeds[s]); ++n) {
      uint8_t *slice = malloc(n);
      ML307_MqttEvent event;
      uint8_t cid; uint16_t mid; uint32_t state;
      CHECK(slice != NULL);
      memcpy(slice, seeds[s], n);
      (void)ML307_MqttParseControlUrc(slice, n, &event);
      (void)ML307_MqttParseSubResponse(slice, n, &cid, &mid);
      (void)ML307_MqttParsePubResponse(slice, n, &cid, &mid);
      (void)ML307_MqttParseStateResponse(slice, n, &state);
      (void)ML307_MqttParseConnectionResponse(slice, n, 0U, &state);
      free(slice);
    }
  }
  uint32_t rng = 12345U;
  for (unsigned i = 0U; i < 10000U; ++i) {
    const size_t n = i % 97U + 1U;
    uint8_t *data = malloc(n);
    uint8_t cid; uint16_t mid; uint32_t state;
    ML307_MqttEvent event;
    CHECK(data != NULL);
    for (size_t j = 0; j < n; ++j) {
      rng = rng * 1664525U + 1013904223U;
      data[j] = (uint8_t)(rng >> 24);
    }
    (void)ML307_MqttParseControlUrc(data, n, &event);
    (void)ML307_MqttParseSubResponse(data, n, &cid, &mid);
    (void)ML307_MqttParsePubResponse(data, n, &cid, &mid);
    (void)ML307_MqttParseStateResponse(data, n, &state);
    (void)ML307_MqttParseConnectionResponse(data, n, 0U, &state);
    free(data);
  }
}

static void test_legacy_non_control_and_builders(void)
{
  ML307_MqttEvent event;
  char out[128];
  CHECK(ML307_MqttParseControlUrc(BYTES("+MQTTURC: \"publish\",0,42,\"a\",5,5,hello"), &event) == ML307_RESULT_NOT_FOUND);
  CHECK(ML307_MqttParseUrc("+MQTTURC: \"publish\",0,42,\"a\",5,5,hello\r\n", &event) == ML307_RESULT_OK);
  CHECK(event.type == ML307_MQTT_EVENT_PUBLISH && !strcmp(event.payload, "hello"));
  CHECK(ML307_MqttParseUrc("+MQTTURC: \"unsuback\",0,42\r\n", &event) == ML307_RESULT_OK);
  CHECK(event.type == ML307_MQTT_EVENT_UNSUBACK);
  CHECK(ML307_MqttBuildSubscribe(out, sizeof(out), 0, "test", 1) == ML307_RESULT_OK);
  CHECK(!strcmp(out, "AT+MQTTSUB=0,\"test\",1\r\n"));
  CHECK(ML307_MqttBuildPublish(out, sizeof(out), 0, "test", 1, 0, "abc") == ML307_RESULT_OK);
  CHECK(!strcmp(out, "AT+MQTTPUB=0,\"test\",1,0,0,3,\"abc\"\r\n"));
}

int main(void)
{
  test_valid_controls();
  test_malformed_controls();
  test_command_responses();
  test_coalesced_connection_responses();
  test_bounded_slices_and_noise();
  test_legacy_non_control_and_builders();
  printf("ML307 control parser: %u checks; bounded slices + 10000 noise inputs passed\n", checks);
  return 0;
}
