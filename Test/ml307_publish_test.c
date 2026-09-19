#include <ML307/ml307_mqtt.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ML307_Result Parse(const char *s, ML307_MqttEvent *e) {
  return ML307_MqttParseUrc(s, e);
}
int main(void) {
  ML307_MqttEvent e;
  const char *bad[] = {"+MQTTURC: \"publish\",0,12,\"topic\",100,100,abc",
                       "+MQTTURC: \"publish\",0,12,\"topic\",-1,-1,x",
                       "+MQTTURC: \"publish\",0,12,\"topic\",4294967296,1,x",
                       "+MQTTURC: \"publish\",0,65536,\"topic\",1,1,x",
                       "+MQTTURC: \"publish\",6,1,\"topic\",1,1,x",
                       "+MQTTURC: \"publish\",0,12,\"topic\",1,2,ab",
                       "+MQTTURC: \"publish\",0,12,\"\",1,1,x",
                       "+MQTTURC: \"publish\",0,12,\"topic\",1,1,xy",
                       "+MQTTURC: \"publish\",0,12,\"topic\",1,1,"};
  for (unsigned i = 0; i < sizeof(bad) / sizeof(*bad); ++i)
    assert(Parse(bad[i], &e) != ML307_RESULT_OK);
  const char good[] = "+MQTTURC: \"publish\",0,12,\"topic\",5,5, a,b \r\n";
  assert(Parse(good, &e) == ML307_RESULT_OK && e.payload_length == 5U &&
         strcmp(e.payload, " a,b ") == 0);
  assert(Parse(" +MQTTURC: \"publish\",0,12,\"t\",0,0,\r\n", &e) == ML307_RESULT_OK);
  assert(Parse("+MQTTURC: \"publish\",0,12,\"t\",10,3,abc", &e) == ML307_RESULT_OK &&
         e.total_length == 10U);
  uint8_t bounded[sizeof(good) - 1U];
  memcpy(bounded, good, sizeof(bounded));
  assert(ML307_MqttParsePublishUrc(bounded, sizeof(bounded), &e) == ML307_RESULT_OK);
  bounded[sizeof(bounded) - 3U] = 0;
  assert(ML307_MqttParsePublishUrc(bounded, sizeof(bounded), &e) != ML307_RESULT_OK &&
         e.type == ML307_MQTT_EVENT_NONE);
  puts("ML307 PUBLISH byte lengths/ranges/empty/fragments: passed");
  return 0;
}
