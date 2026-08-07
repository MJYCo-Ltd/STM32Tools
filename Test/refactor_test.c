#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "Common.h"
#include "ECSense.h"
#include "Flash/nor_flash.h"
#include "Protocol/modbus_codec.h"

static void TestNorHelpers(void)
{
  assert(NorFlash_CapacityFromJedec(UINT32_C(0xEF4015)) ==
         UINT32_C(2) * 1024U * 1024U);
  assert(NorFlash_CapacityFromJedec(UINT32_C(0xEF4020)) == 0U);
  assert(NorFlash_CheckRange(1024U, 1000U, 24U) == NOR_FLASH_OK);
  assert(NorFlash_CheckRange(1024U, 1000U, 25U) == NOR_FLASH_ERR_RANGE);
  assert(NorFlash_PageChunk(250U, 20U, 256U) == 6U);
  assert(NorFlash_AlignDown(5000U, 4096U) == 4096U);
}

static void TestModbusAndECSense(void)
{
  uint8_t request[16];
  uint8_t response[21] = {0};
  ECSense_DS4_Value value;
  uint32_t bits;
  float smooth = 12.5F;
  float real = 12.25F;
  char text[ECSENSE_SHOW_INFO_BUFFER_SIZE];
  char small[8];
  size_t length;

  length = ReadDS4Value(1U, request);
  assert(length == 8U);
  assert(request[0] == 1U);
  assert(request[1] == 0x03U);
  assert(Modbus_ValidateFrame(request, length));
  assert(ReadDS4Value(0U, request) == 0U);
  assert(ReadDS4Value(1U, NULL) == 0U);

  assert(DS4Sleep(2U, request) == 11U);
  assert(Modbus_ValidateFrame(request, 11U));
  assert(DS4Wakeup(3U, request) == 13U);
  assert(request[0] == 3U);
  assert(request[8] == 3U);
  assert(Modbus_ValidateFrame(request, 13U));

  response[0] = 1U;
  response[1] = 0x03U;
  response[2] = 16U;
  memcpy(&bits, &smooth, sizeof(bits));
  WriteBE32(response + 3U, bits);
  WriteBE16(response + 7U, 100U);
  response[10] = ECSENSE_CO;
  response[12] = ECSENSE_PPM;
  response[14] = 0U;
  memcpy(&bits, &real, sizeof(bits));
  WriteBE32(response + 15U, bits);
  assert(Modbus_AppendCrc(response, 19U, sizeof(response)) ==
         sizeof(response));
  assert(ReadDS4ValueResponse(response, sizeof(response), &value) == 1U);
  assert(value.fSmoothValue == smooth);
  assert(value.fRealValue == real);
  assert(value.uMaxRange == 100U);
  assert(GetShowInfoSized(&value, text, sizeof(text)) > 0U);
  assert(GetShowInfoSized(&value, small, sizeof(small)) == 0U);
}

int main(void)
{
  TestNorHelpers();
  TestModbusAndECSense();
  return 0;
}
