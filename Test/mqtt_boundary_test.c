#include <ML307/ml307_mqtt.h>
#include <Protocol/MqttLineCollector.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned calls, accepted;
static char captured[MQTT_LINE_COLLECTOR_CAPACITY];
static void Receive(char *line, void *context)
{
  ML307_MqttEvent event;
  (void)context;
  ++calls;
  strcpy(captured, line);
  if (ML307_MqttParseUrc(line, &event) == ML307_RESULT_OK) ++accepted;
}
static void Overflow(void)
{
  MqttLineCollector c;
  uint8_t prefix[MQTT_LINE_COLLECTOR_CAPACITY - 1U];
  const char suffix[] = "+MQTTURC: \"suback\",0,42,1\r\n";
  memset(prefix, 'x', sizeof(prefix));
  calls = accepted = 0U;
  MqttLineCollector_Init(&c, Receive, NULL);
  MqttLineCollector_Feed(&c, prefix, sizeof(prefix));
  for (unsigned i=0; i<sizeof(suffix)-1U; ++i)
    MqttLineCollector_Feed(&c, (const uint8_t *)suffix+i, 1U);
  assert(calls == 0U && accepted == 0U);
  MqttLineCollector_Feed(&c, (const uint8_t *)suffix, sizeof(suffix)-1U);
  assert(calls == 1U && accepted == 1U && !strcmp(captured, suffix));
  const uint8_t hidden[] = {'x',0,'+', 'M','Q','T','T','U','R','C',':','\n'};
  MqttLineCollector_Feed(&c, hidden, sizeof(hidden));
  assert(calls == 1U);
}
static void Publish(void)
{
  static const char *bad[] = {
    "+MQTTURC: \"publish\",0,12,\"topic\",100,100,abc",
    "+MQTTURC: \"publish\",0,12,\"topic\",-1,-1,abc",
    "+MQTTURC: \"publish\",0,12,\"topic\",2,3,abc",
    "+MQTTURC: \"publish\",0,65536,\"topic\",3,3,abc",
    "+MQTTURC: \"publish\",6,12,\"topic\",3,3,abc",
    "+MQTTURC: \"publish\",0,12,\"topic\",4294967296,3,abc",
    "+MQTTURC: \"publish\",0,12,\"topic\",3,3,abcd",
    "+MQTTURC: \"publish\",0,12,\"topic\",0,0,x"
  };
  ML307_MqttEvent e;
  for (unsigned i=0; i<sizeof(bad)/sizeof(bad[0]); ++i) {
    memset(&e, 0xA5, sizeof(e));
    assert(ML307_MqttParseUrc(bad[i], &e) != ML307_RESULT_OK);
    assert(e.type == ML307_MQTT_EVENT_NONE);
  }
  const char good[] = "+MQTTURC: \"publish\",0,12,\"topic\",5,5, a,b \r\n";
  assert(ML307_MqttParseUrc(good, &e) == ML307_RESULT_OK);
  assert(e.total_length == 5U && e.payload_length == 5U && !strcmp(e.payload, " a,b "));
  const char partial[] = "+MQTTURC: \"publish\",0,12,\"topic\",10,3,abc";
  assert(ML307_MqttParseTextPublish((const uint8_t *)partial, sizeof(partial)-1U,&e)==ML307_RESULT_OK);
  assert(e.payload_length < e.total_length);
  const char empty[] = "+MQTTURC: \"publish\",0,0,\"topic\",0,0,";
  assert(ML307_MqttParseUrc(empty, &e)==ML307_RESULT_OK && e.payload[0]==0);
  /* All prefix truncations are bounded, including exact-size non-NUL slices. */
  for (size_t n=0;n<sizeof(good)-3U;++n) {
    uint8_t buffer[sizeof(good)]; memcpy(buffer,good,n);
    assert(ML307_MqttParseTextPublish(buffer,n,&e)!=ML307_RESULT_OK);
  }
}
int main(void) { Overflow(); Publish(); puts("MQTT boundary regressions passed"); return 0; }
