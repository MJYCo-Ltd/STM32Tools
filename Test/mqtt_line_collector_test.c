#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <Protocol/MqttLineCollector.h>
#include <EWM103/ewm103_parser.h>
typedef struct { unsigned calls; char last[MQTT_LINE_COLLECTOR_CAPACITY]; } Capture;
static void CaptureLine(char *line,void *ctx) { Capture *c=ctx;++c->calls;strcpy(c->last,line); }

static unsigned frames, text_lines;
static size_t text_bytes;
static char order[32];
static unsigned order_length;
static void frame(char *data,size_t length,void *context) {
  EWM103_MqttMessage message;
  (void)context;
  assert(EWM103_ParseMqttReceive((const uint8_t *)data,length,&message)==MODULE_FRAME_COMPLETE);
  assert(message.link==0U && message.topic.length==3U && !memcmp(message.topic.data,"a,b",3U));
  ++frames;order[order_length++]='F';
}
static void text(const uint8_t *data,size_t length,void *context) {
  (void)data;(void)context;text_bytes+=length;
}
static void line(char *data,void *context) {
  (void)data;(void)context;++text_lines;order[order_length++]='T';
}
static void reset(MqttLineCollector *c) {
  MqttLineCollector_Init(c,line,NULL);frames=text_lines=order_length=0;text_bytes=0;
}
static void feed(MqttLineCollector *c,const void *data,size_t length) {
  assert(length<=UINT16_MAX);
  MqttLineCollector_FeedFrames(c,data,(uint16_t)length,&EWM103_MqttReceiveProtocol,frame,text);
}
static void FramedTests(void) {
  static const char body[]="\r\nOK\r\nERROR\r\nWIFI DISCONNECT\r\n+MQTTDISCONNECTED:0\r\n\0suffix";
  char packet[512], oversized[800];
  MqttLineCollector c;
  EWM103_MqttMessage message;
  size_t header=(size_t)snprintf(packet,sizeof(packet),"+MQTTSUBRECV:0,\"a,b\",%u,",(unsigned)(sizeof(body)-1U));
  size_t length=header+sizeof(body)-1U+2U;
  memcpy(packet+header,body,sizeof(body)-1U);memcpy(packet+length-2U,"\r\n",2U);
  for(size_t n=0;n<length;++n) {
    reset(&c);feed(&c,packet,n);
    assert(!frames && !text_bytes);
    assert(EWM103_ParseMqttReceive((const uint8_t *)packet,n,&message)!=MODULE_FRAME_COMPLETE);
    feed(&c,packet+n,length-n);assert(frames==1U && !text_bytes && !text_lines);
  }
  assert(EWM103_ParseMqttReceive((const uint8_t *)packet,length,&message)==MODULE_FRAME_COMPLETE);
  assert(message.payload.length==sizeof(body)-1U && !memcmp(message.payload.data,body,sizeof(body)-1U));
  reset(&c);feed(&c,packet,length);feed(&c,"OK\r\n",4U);feed(&c,packet,length);
  assert(frames==2U && text_lines==1U && text_bytes==4U && !memcmp(order,"FTF",3U));
  header=(size_t)snprintf(oversized,sizeof(oversized),"+MQTTSUBRECV:0,\"a,b\",600,");
  for(size_t n=0;n<600U;++n) oversized[header+n]="\r\nOK\0ERROR"[n%10U];
  memcpy(oversized+header+600U,"\r\n",2U);
  for(size_t n=0;n<header+602U;++n) {
    reset(&c);feed(&c,oversized,n);feed(&c,oversized+n,header+602U-n);
    assert(!frames && !text_bytes);feed(&c,packet,length);assert(frames==1U && !text_bytes);
  }
  reset(&c);feed(&c,"+MQTTSUBRECV:0,\"a,b\",4294967296,x\r\n",strlen("+MQTTSUBRECV:0,\"a,b\",4294967296,x\r\n"));
  feed(&c,packet,length);assert(frames==1U && !text_bytes);
  reset(&c);feed(&c,"+MQTTSUBRECV:0,\"a,b\",3,abcXXignored\n",strlen("+MQTTSUBRECV:0,\"a,b\",3,abcXXignored\n"));
  feed(&c,packet,length);assert(frames==1U && !text_bytes);
  reset(&c);feed(&c,"+MQTTSUBRECV:0,\"unterminated",strlen("+MQTTSUBRECV:0,\"unterminated"));
  memset(oversized,'x',200U);feed(&c,oversized,200U);feed(&c,"\n",1U);
  feed(&c,packet,length);assert(frames==1U && !text_bytes);
  /* Existing numeric-only protocol initializers need no new fields. */
  const ModuleLengthFrameProtocol numeric={.prefix="+DATA:",.field_count=2U,
    .payload_length_field=1U,.field_separator=',',.maximum_payload_length=16U,
    .fixed_tail="\r\n",.fixed_tail_length=2U};
  uint32_t fields[2];const uint8_t *payload;size_t payload_length,consumed;
  const char raw[]="+DATA:2,4,\0\nOK\r\n";
  assert(ModuleFrameParser_ParseLengthFrame((const uint8_t *)raw,sizeof(raw)-1U,&numeric,
    fields,2U,&payload,&payload_length,&consumed)==MODULE_FRAME_COMPLETE);
  assert(fields[0]==2U && payload_length==4U && consumed==sizeof(raw)-1U && payload[0]==0U);
  assert(EWM103_ParseLinkEvent((const uint8_t *)"WIFI GOT IP\r\n",13U)==EWM103_LINK_EVENT_GOT_IP);
  assert(EWM103_ParseLinkEvent((const uint8_t *)"WIFI DISCONNECT\r\n",17U)==EWM103_LINK_EVENT_DISCONNECTED);
  assert(EWM103_ParseLinkEvent((const uint8_t *)"+MQTTDISCONNECTED:0\r\n",21U)==EWM103_LINK_EVENT_MQTT_DISCONNECTED);
}
int main(void) {
  FramedTests();
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
