#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "ML307/ml307_http.h"

static void TestBuilders(void)
{
  char packet[128];
  size_t length;
  assert(ML307_HttpBuildCreate(packet, sizeof(packet),
                               "http://ota.example.com:8080", &length) == ML307_RESULT_OK);
  assert(length == strlen(packet));
  assert(strcmp(packet, "AT+MHTTPCREATE=\"http://ota.example.com:8080\"\r\n") == 0);
  assert(ML307_HttpBuildCached(packet, sizeof(packet), 0U, 1U, &length) == ML307_RESULT_OK);
  assert(strcmp(packet, "AT+MHTTPCFG=\"cached\",0,1\r\n") == 0);
  assert(ML307_HttpBuildSsl(packet, sizeof(packet), 0U, 1U, 0U, &length) == ML307_RESULT_OK);
  assert(strcmp(packet, "AT+MHTTPCFG=\"ssl\",0,1,0\r\n") == 0);
  assert(ML307_HttpBuildSsl(packet, sizeof(packet), 0U, 1U, 6U, &length) ==
         ML307_RESULT_INVALID_VALUE);
  assert(ML307_HttpBuildHeader(packet, sizeof(packet), 0U,
                               "Range: bytes=256-511", &length) == ML307_RESULT_OK);
  assert(ML307_HttpBuildGet(packet, sizeof(packet), 0U,
                            "/device/ota/file/42", &length) == ML307_RESULT_OK);
  assert(strcmp(packet, "AT+MHTTPREQUEST=0,1,0,\"/device/ota/file/42\"\r\n") == 0);
  assert(ML307_HttpBuildRead(packet, sizeof(packet), 0U, 1U, 256U, &length) == ML307_RESULT_OK);
  assert(strcmp(packet, "AT+MHTTPREAD=0,1,256\r\n") == 0);
  assert(ML307_HttpBuildDelete(packet, sizeof(packet), 0U, &length) == ML307_RESULT_OK);
  assert(ML307_HttpBuildHeader(packet, sizeof(packet), 0U,
                               "Range: bytes=0-1\r\nBAD", &length) == ML307_RESULT_INVALID_ARGUMENT);
}

static void TestParsers(void)
{
  uint8_t id = 0xFFU;
  ML307_HttpRecvEvent event;
  assert(ML307_HttpParseCreate("\r\n+MHTTPCREATE: 2\r\nOK\r\n", &id) == ML307_RESULT_OK);
  assert(id == 2U);
  assert(ML307_HttpParseCreate("+MHTTPCREATE: 9\r\nOK\r\n", &id) ==
         ML307_RESULT_INVALID_VALUE);
  assert(ML307_HttpParseRecvUrc("+MHTTPURC: \"recv\",2,206,188,256\r\n",
                                &event) == ML307_RESULT_OK);
  assert(event.http_id == 2U && event.status_code == 206U);
  assert(event.header_length == 188U && event.content_length == 256U);
}

static void TestBinaryRead(void)
{
  static const uint8_t prefix[] = "\r\n+MHTTPREAD: 0,1,7,5,";
  static const uint8_t suffix[] = "\r\nOK\r\n";
  const uint8_t binary[] = {0x00U, 0x2CU, 0x0AU, 0x0DU, 0xFFU};
  uint8_t response[sizeof(prefix) - 1U + sizeof(binary) + sizeof(suffix) - 1U];
  uint8_t output[8];
  size_t offset = 0U, data_length = 0U;
  uint32_t unread = 0U;
  memcpy(response + offset, prefix, sizeof(prefix) - 1U); offset += sizeof(prefix) - 1U;
  memcpy(response + offset, binary, sizeof(binary)); offset += sizeof(binary);
  memcpy(response + offset, suffix, sizeof(suffix) - 1U); offset += sizeof(suffix) - 1U;
  assert(ML307_HttpParseRead(response, offset, 0U, 1U, output, sizeof(output),
                             &data_length, &unread) == ML307_RESULT_OK);
  assert(data_length == sizeof(binary) && unread == 7U);
  assert(memcmp(output, binary, sizeof(binary)) == 0);
  assert(ML307_HttpParseRead(response, offset, 0U, 1U, output, 4U,
                             &data_length, &unread) == ML307_RESULT_BUFFER_TOO_SMALL);
  assert(ML307_HttpParseRead(response, offset - 2U, 0U, 1U, output,
                             sizeof(output), &data_length, &unread) ==
         ML307_RESULT_NOT_FOUND);
}

int main(void)
{
  TestBuilders();
  TestParsers();
  TestBinaryRead();
  return 0;
}
