#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <Protocol/MqttLineCollector.h>
typedef struct { unsigned calls; char last[MQTT_LINE_COLLECTOR_CAPACITY]; } Capture;
static void CaptureLine(char *line,void *ctx) { Capture *c=ctx;++c->calls;strcpy(c->last,line); }
int main(void) {
  MqttLineCollector c; Capture out={0};
  MqttLineCollector_Init(&c,CaptureLine,&out);
  MqttLineCollector_Feed(&c,(const uint8_t *)"+MQTT",5U);assert(out.calls==0U);
  MqttLineCollector_Feed(&c,(const uint8_t *)":payload\r\n",10U);
  assert(out.calls==1U && strcmp(out.last,"+MQTT:payload\r\n")==0);
  MqttLineCollector_Feed(&c,(const uint8_t *)"one\ntwo\n",8U);
  assert(out.calls==3U && strcmp(out.last,"two\n")==0);
  uint8_t noise[MQTT_LINE_COLLECTOR_CAPACITY-1U];memset(noise,'x',sizeof(noise));
  for(unsigned split=0;split<sizeof(noise);++split) {
    out.calls=0U;MqttLineCollector_Init(&c,CaptureLine,&out);
    MqttLineCollector_Feed(&c,noise,(uint16_t)split);
    MqttLineCollector_Feed(&c,noise+split,(uint16_t)(sizeof(noise)-split));
    const char *tail="+MQTTURC: \"suback\",0,42,0\r\n";
    MqttLineCollector_Feed(&c,(const uint8_t *)tail,(uint16_t)strlen(tail));
    assert(out.calls==0U);
    MqttLineCollector_Feed(&c,(const uint8_t *)"next\n",5U);
    assert(out.calls==1U && strcmp(out.last,"next\n")==0);
  }
  out.calls=0U;MqttLineCollector_Init(&c,CaptureLine,&out);
  const uint8_t nul_line[]={'O','K',0,'x','\n','a','\n'};
  MqttLineCollector_Feed(&c,nul_line,sizeof(nul_line));assert(out.calls==1U && strcmp(out.last,"a\n")==0);
  MqttLineCollector_Invalidate(&c);MqttLineCollector_Feed(&c,(const uint8_t *)"tail\nnext\n",10U);
  assert(out.calls==2U && strcmp(out.last,"next\n")==0);
  puts("MQTT collector overflow/NUL/fragments/legacy terminators: passed");return 0;
}
