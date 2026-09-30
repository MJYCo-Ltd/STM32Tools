#include <EWM103/ewm103.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Enumerate every proper prefix, including both bytes of the final CRLF. */
static void EveryBoundary(const char *response)
{
  char buffer[256];
  const size_t length = strlen(response);
  size_t split;
  assert(length < sizeof(buffer));
  for (split = 0U; split < length; ++split) {
    memcpy(buffer, response, split);
    buffer[split] = '\0';
    assert(!EWM103_IsComplete(buffer, EWM103_TYPE_CWJAP));
    memcpy(buffer + split, response + split, length - split + 1U);
    assert(EWM103_IsComplete(buffer, EWM103_TYPE_CWJAP));
  }
}

int main(void)
{
  EWM103_Data data;
  const char *query = "\r\n+CWJAP:\"field\",\"00:11:22:33:44:55\",1,-57\r\nOK\r\n";
  EveryBoundary(query);
  EveryBoundary("\r\nNo AP\r\nOK\r\n");
  EveryBoundary("\r\n+CWJAP:1\r\nERROR\r\n");
  EveryBoundary("\r\nERROR\r\n");
  EveryBoundary("\r\n+CME ERROR:1\r\n");
  EveryBoundary("\r\n+CMS ERROR:1\r\n");
  EveryBoundary("\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n");
  assert(!EWM103_IsComplete("+CWJAP:", EWM103_TYPE_CWJAP));
  assert(!EWM103_IsComplete("+CWJAP:\"WIFI GOT IP\",\"OK\",1,-57\r\n",
                            EWM103_TYPE_CWJAP));
  assert(!EWM103_IsComplete("+MQTTSUBRECV:0,\"x\",11,WIFI GOT IP\r\n",
                            EWM103_TYPE_CWJAP));
  assert(!EWM103_IsComplete(NULL, EWM103_TYPE_CWJAP));
  assert(EWM103_IsComplete("WIFI GOT IP\n", EWM103_TYPE_CWJAP));
  assert(EWM103_IsComplete("OK\n", EWM103_TYPE_CWJAP));
  assert(EWM103_Unpack(query, EWM103_TYPE_CWJAP, &data) == EWM103_RESULT_OK);
  assert(strcmp(data.text, "\"field\",\"00:11:22:33:44:55\",1,-57") == 0);
  puts("ewm103_cwjap_fragment_test: all byte boundaries and join URCs passed");
  return 0;
}
