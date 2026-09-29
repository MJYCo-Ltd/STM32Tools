#include <ML307/ml307_http.h>
#include <ML307/ml307_mqtt_ack.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static ML307_HttpReadLimits limits = {512U, 128U};
static uint8_t output[512];
static ML307_HttpReadResult result;

static ML307_Result Read(const uint8_t *data, size_t n, uint8_t id)
{
  return ML307_HttpParseReadFrame(data, n, id, 1U, &limits,
                                 output, sizeof(output), &result);
}

static void TestUrc(void)
{
  ML307_HttpUrcEvent event;
  const char *valid = "+MHTTPURC: \"recv\",3,999,4294967295,4294967295\r\n";
  assert(ML307_HttpParseUrcLine((const uint8_t *)valid, strlen(valid), &event) == ML307_RESULT_OK);
  assert(event.kind == ML307_HTTP_URC_RECV && event.http_id == 3U);
  assert(event.status_code == 999U && event.header_length == UINT32_MAX && event.content_length == UINT32_MAX);
  const char *bad[] = {
    "+MHTTPURC: \"recv\",4,206,1,2", "+MHTTPURC: \"recv\",0,1000,1,2",
    "+MHTTPURC: \"recv\",0,206,4294967296,2", "+MHTTPURC: \"recv\",0,206,1,-2",
    "+MHTTPURC: \"recv\",0,206,1,2,9", "+MHTTPURC: \"err\",0,256",
    "+MHTTPURC: \"err\",0,5\rjunk", "+MHTTPURC: \"err\",0,5\r\nx",
    "+MHTTPURC: \"err\",0", "+MHTTPURC: \"err\",+0,5",
    "junk+MHTTPURC: \"err\",0,5", "+MHTTPURC: \"header\",0,206,10,HTTP"
  };
  for (size_t i=0U; i<sizeof(bad)/sizeof(bad[0]); ++i) {
    memset(&event, 0xFF, sizeof(event));
    assert(ML307_HttpParseUrcLine((const uint8_t *)bad[i], strlen(bad[i]), &event) != ML307_RESULT_OK);
    assert(event.kind == ML307_HTTP_URC_NONE);
  }
  const char *endings[] = {"", "\r", "\n", "\r\n"};
  for (size_t i=0U; i<4U; ++i) {
    char line[64]; snprintf(line,sizeof(line),"+MHTTPURC: \"err\",1,255%s",endings[i]);
    assert(ML307_HttpParseUrcLine((const uint8_t *)line,strlen(line),&event)==ML307_RESULT_OK);
    assert(event.kind==ML307_HTTP_URC_ERR && event.http_id==1U && event.error_code==255U);
  }
  const uint8_t nul[] = "+MHTTPURC: \"err\",0,5\0junk";
  assert(ML307_HttpParseUrcLine(nul,sizeof(nul)-1U,&event)==ML307_RESULT_INVALID_VALUE);
  assert(ML307_HttpParseUrcLine(NULL,0U,&event)==ML307_RESULT_INVALID_ARGUMENT);
}

static void TestOpaqueBody(void)
{
  const uint8_t body[] = "\0,\r\nOK\r\nERROR\r\n+MHTTPURC: \"err\",0,5\r\n+MHTTPREAD: 0,1,0,1,x";
  uint8_t frame[256];
  int prefix=snprintf((char *)frame,sizeof(frame),"\r\n+MHTTPREAD: 0,1,7,%u,",(unsigned)(sizeof(body)-1U));
  assert(prefix>0);
  size_t length=(size_t)prefix;
  memcpy(frame+length,body,sizeof(body)-1U); length+=sizeof(body)-1U;
  memcpy(frame+length,"\r\nOK\r\n",6U); length+=6U;
  for (size_t n=0U; n<length; ++n) {
    memset(output,0xA5,sizeof(output));
    assert(Read(frame,n,0U)==ML307_RESULT_NOT_FOUND);
    assert(output[0]==0xA5 && result.data_length==0U && !result.has_module_error);
  }
  assert(Read(frame,length,0U)==ML307_RESULT_OK);
  assert(result.data_length==sizeof(body)-1U && result.unread_length==7U && result.consumed==length);
  assert(memcmp(output,body,sizeof(body)-1U)==0);
  assert(Read(frame,length,1U)==ML307_RESULT_INVALID_VALUE);
  limits.maximum_payload_length=1U;
  assert(Read(frame,length,0U)==ML307_RESULT_INVALID_VALUE);
  limits.maximum_payload_length=512U;
  assert(ML307_HttpParseReadFrame(frame,length,0U,1U,&limits,output,1U,&result)==ML307_RESULT_BUFFER_TOO_SMALL);
  assert(!result.data_length);
}

static void TestInterleavedTail(void)
{
  const char *ok = "+MHTTPREAD: 0,1,0,1,x\r\n\r\n+MHTTPURC: \"recv\",0,206,2,1\r\n+MHTTPURC: \"err\",1,5\r\nOK\r\n";
  assert(Read((const uint8_t *)ok,strlen(ok),0U)==ML307_RESULT_OK);
  assert(result.data_length==1U && output[0]=='x' && !result.has_module_error);
  const char *err = "+MHTTPREAD: 0,1,0,1,x\r\n+MHTTPURC: \"err\",0,0\r\nOK\r\n";
  assert(Read((const uint8_t *)err,strlen(err),0U)==ML307_RESULT_INVALID_VALUE);
  assert(result.has_module_error && result.module_error==0U && !result.data_length);
  /* No global parser context: a subsequent independent instance has no stale error. */
  assert(Read((const uint8_t *)ok,strlen(ok),0U)==ML307_RESULT_OK && !result.has_module_error);
  const char *bad[] = {
    "+MHTTPREAD: 0,1,0,1,x\r\nERROR\r\n",
    "+MHTTPREAD: 0,1,0,1,x\r\n+CME ERROR: 5\r\n",
    "+MHTTPREAD: 0,1,0,1,x\r\nGARBAGE\r\nOK\r\n",
    "+MHTTPREAD: 0,1,0,1,x\r\n+MHTTPURC: \"err\",0,999\r\nOK\r\n",
    "+MHTTPREAD: 0,1,0,4294967296,x\r\nOK\r\n"
  };
  for(size_t i=0U;i<sizeof(bad)/sizeof(bad[0]);++i)
    assert(Read((const uint8_t *)bad[i],strlen(bad[i]),0U)==ML307_RESULT_INVALID_VALUE);
  const char *error="\r\nERROR\r\n";
  assert(Read((const uint8_t *)error,strlen(error),0U)==ML307_RESULT_ERROR_RESPONSE);
  const char *body_incomplete="+MHTTPREAD: 0,1,0,100,\r\nERROR\r\n";
  assert(Read((const uint8_t *)body_incomplete,strlen(body_incomplete),0U)==ML307_RESULT_NOT_FOUND);
  limits.urc_line_capacity=10U;
  assert(Read((const uint8_t *)ok,strlen(ok),0U)==ML307_RESULT_INVALID_VALUE);
  limits.urc_line_capacity=128U;
}

static void TestBrokerAck(void)
{
  ML307_MqttEvent event={0};
  event.connect_id=2U; event.message_id=123U; event.type=ML307_MQTT_EVENT_SUBACK;
  for (int s=0;s<=2;++s) { event.state=s; assert(ML307_MqttClassifyAck(&event,1U,2U,123U)==ML307_MQTT_ACK_SUCCESS); }
  event.state=128; assert(ML307_MqttClassifyAck(&event,1U,2U,123U)==ML307_MQTT_ACK_FAILURE);
  assert(ML307_MqttClassifyAck(&event,1U,2U,124U)==ML307_MQTT_ACK_IGNORE);
  assert(ML307_MqttClassifyAck(&event,0U,2U,123U)==ML307_MQTT_ACK_IGNORE);
  event.type=ML307_MQTT_EVENT_TIMEOUT;
  assert(ML307_MqttClassifyAck(&event,0U,2U,123U)==ML307_MQTT_ACK_FAILURE);
  event.type=ML307_MQTT_EVENT_PUBACK; event.state=1;
  assert(ML307_MqttClassifyAck(&event,0U,2U,123U)==ML307_MQTT_ACK_SUCCESS);
  event.state=2; assert(ML307_MqttClassifyAck(&event,0U,2U,123U)==ML307_MQTT_ACK_IGNORE);
  assert(ML307_MqttClassifyAck(NULL,0U,2U,123U)==ML307_MQTT_ACK_IGNORE);
}

int main(void)
{
  TestUrc(); TestOpaqueBody(); TestInterleavedTail(); TestBrokerAck();
  puts("ML307 bounded HTTP/URC and typed broker ACK migration PASS");
  return 0;
}
