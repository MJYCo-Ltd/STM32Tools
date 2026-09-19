#include <AHT20/aht20.h>
#include <TMP/tmp117.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FAKE_RX_CAPACITY 256U
#define FAKE_TX_CALLS 32U
#define FAKE_TX_BYTES 8U

typedef struct {
  uint8_t expected_address;
  I2C_BusResult transmit_result;
  I2C_BusResult receive_result;
  uint8_t receive_bytes[FAKE_RX_CAPACITY];
  size_t receive_size;
  size_t receive_offset;
  uint8_t transmit_bytes[FAKE_TX_CALLS][FAKE_TX_BYTES];
  size_t transmit_lengths[FAKE_TX_CALLS];
  size_t transmit_calls;
  size_t receive_calls;
} FakeI2C;

typedef struct {
  uint32_t total_ms;
  uint32_t calls;
} FakeDelay;

static I2C_BusResult FakeTransmit(void *handle, uint8_t address7,
                                  uint8_t *data, size_t length,
                                  uint32_t timeout_ms)
{
  FakeI2C *fake = handle;
  (void)timeout_ms;

  if ((fake == NULL) || (data == NULL) || (length == 0U) ||
      (address7 != fake->expected_address)) {
    return I2C_BUS_IO_ERROR;
  }
  if (fake->transmit_result != I2C_BUS_OK) {
    return fake->transmit_result;
  }
  if ((fake->transmit_calls >= FAKE_TX_CALLS) ||
      (length > FAKE_TX_BYTES)) {
    return I2C_BUS_IO_ERROR;
  }

  memcpy(fake->transmit_bytes[fake->transmit_calls], data, length);
  fake->transmit_lengths[fake->transmit_calls] = length;
  ++fake->transmit_calls;
  return I2C_BUS_OK;
}

static I2C_BusResult FakeReceive(void *handle, uint8_t address7,
                                 uint8_t *data, size_t length,
                                 uint32_t timeout_ms)
{
  FakeI2C *fake = handle;
  (void)timeout_ms;

  if ((fake == NULL) || (data == NULL) || (length == 0U) ||
      (address7 != fake->expected_address)) {
    return I2C_BUS_IO_ERROR;
  }
  if (fake->receive_result != I2C_BUS_OK) {
    return fake->receive_result;
  }
  if ((fake->receive_offset + length) > fake->receive_size) {
    return I2C_BUS_IO_ERROR;
  }

  memcpy(data, fake->receive_bytes + fake->receive_offset, length);
  fake->receive_offset += length;
  ++fake->receive_calls;
  return I2C_BUS_OK;
}

static void FakeInit(FakeI2C *fake, uint8_t address7)
{
  memset(fake, 0, sizeof(*fake));
  fake->expected_address = address7;
  fake->transmit_result = I2C_BUS_OK;
  fake->receive_result = I2C_BUS_OK;
}

static I2C_Bus FakeMakeBus(FakeI2C *fake)
{
  I2C_Bus bus;
  bus.handle = fake;
  bus.transmit = FakeTransmit;
  bus.receive = FakeReceive;
  bus.timeout_ms = 100U;
  return bus;
}

static void FakePushReceive(FakeI2C *fake, const void *data, size_t length)
{
  assert(fake != NULL);
  assert(data != NULL);
  assert((fake->receive_size + length) <= sizeof(fake->receive_bytes));
  memcpy(fake->receive_bytes + fake->receive_size, data, length);
  fake->receive_size += length;
}

static void FakePushU16(FakeI2C *fake, uint16_t value)
{
  const uint8_t bytes[2] = {(uint8_t)(value >> 8U), (uint8_t)value};
  FakePushReceive(fake, bytes, sizeof(bytes));
}

static void FakeDelayMs(void *context, uint32_t delay_ms)
{
  FakeDelay *delay = context;
  assert(delay != NULL);
  delay->total_ms += delay_ms;
  ++delay->calls;
}

static uint8_t AhtCrc8(const uint8_t *data, size_t length)
{
  uint8_t crc = 0xFFU;
  size_t byte;
  uint8_t bit;

  for (byte = 0U; byte < length; ++byte) {
    crc ^= data[byte];
    for (bit = 0U; bit < 8U; ++bit) {
      crc = ((crc & 0x80U) != 0U) ? (uint8_t)((crc << 1U) ^ 0x31U)
                                  : (uint8_t)(crc << 1U);
    }
  }
  return crc;
}

static void PushAhtMeasurement(FakeI2C *fake, uint32_t humidity_raw,
                               uint32_t temperature_raw, uint8_t corrupt_crc)
{
  uint8_t status = 0x08U;
  uint8_t frame[7];

  FakePushReceive(fake, &status, 1U);
  frame[0] = status;
  frame[1] = (uint8_t)(humidity_raw >> 12U);
  frame[2] = (uint8_t)(humidity_raw >> 4U);
  frame[3] = (uint8_t)((humidity_raw << 4U) & 0xF0U) |
             (uint8_t)((temperature_raw >> 16U) & 0x0FU);
  frame[4] = (uint8_t)(temperature_raw >> 8U);
  frame[5] = (uint8_t)temperature_raw;
  frame[6] = AhtCrc8(frame, 6U);
  if (corrupt_crc != 0U) {
    frame[6] ^= 0x01U;
  }
  FakePushReceive(fake, frame, sizeof(frame));
}

static void AssertFloatNear(float actual, float expected, float tolerance)
{
  const float difference =
      (actual >= expected) ? (actual - expected) : (expected - actual);
  assert(difference <= tolerance);
}

static void test_aht20_requires_explicit_dependencies(void)
{
  AHT20_Device device;
  AHT20_Data data = {.valid = true};
  I2C_Bus invalid_bus = {0};
  FakeI2C fake;
  I2C_Bus bus;

  memset(&device, 0, sizeof(device));
  FakeInit(&fake, AHT20_I2C_ADDR7);
  bus = FakeMakeBus(&fake);

  assert(AHT20_DeviceInit(NULL, &bus, AHT20_I2C_ADDR7, FakeDelayMs,
                          NULL) == AHT20_ERR_PARAM);
  assert(AHT20_DeviceInit(&device, NULL, AHT20_I2C_ADDR7, FakeDelayMs,
                          NULL) == AHT20_ERR_PARAM);
  assert(AHT20_DeviceInit(&device, &invalid_bus, AHT20_I2C_ADDR7,
                          FakeDelayMs, NULL) == AHT20_ERR_PARAM);
  assert(AHT20_DeviceInit(&device, &bus, AHT20_I2C_ADDR7, NULL,
                          NULL) == AHT20_ERR_PARAM);
  assert(AHT20_Initialize(&device) == AHT20_ERR_PARAM);
  assert(AHT20_Read(&device, &data) == AHT20_ERR_PARAM);
  assert(data.valid == false);
}

static void test_aht20_initializes_and_reads_known_values(void)
{
  FakeI2C fake;
  I2C_Bus bus;
  FakeDelay delay = {0};
  AHT20_Device device;
  AHT20_Data data;
  const uint8_t calibrated = 0x08U;

  FakeInit(&fake, AHT20_I2C_ADDR7);
  bus = FakeMakeBus(&fake);
  FakePushReceive(&fake, &calibrated, 1U);

  assert(AHT20_DeviceInit(&device, &bus, AHT20_I2C_ADDR7, FakeDelayMs,
                          &delay) == AHT20_OK);
  assert(AHT20_Initialize(&device) == AHT20_OK);
  assert(delay.total_ms == 40U);
  assert(fake.transmit_calls == 0U);

  PushAhtMeasurement(&fake, 0x80000UL, 0x60000UL, 0U);
  assert(AHT20_Read(&device, &data) == AHT20_OK);
  assert(data.valid);
  AssertFloatNear(data.humidity_rh, 50.0f, 0.01f);
  AssertFloatNear(data.temperature_c, 25.0f, 0.01f);
  assert(delay.total_ms == 120U);
  assert(fake.transmit_calls == 1U);
  assert(fake.transmit_lengths[0] == 3U);
  assert(fake.transmit_bytes[0][0] == 0xACU);
  assert(fake.transmit_bytes[0][1] == 0x33U);
  assert(fake.transmit_bytes[0][2] == 0x00U);
}

static void test_aht20_calibration_and_errors(void)
{
  FakeI2C fake;
  I2C_Bus bus;
  FakeDelay delay = {0};
  AHT20_Device device;
  AHT20_Data data;
  const uint8_t status_sequence[2] = {0x00U, 0x08U};

  FakeInit(&fake, AHT20_I2C_ADDR7);
  bus = FakeMakeBus(&fake);
  FakePushReceive(&fake, status_sequence, sizeof(status_sequence));
  assert(AHT20_DeviceInit(&device, &bus, AHT20_I2C_ADDR7, FakeDelayMs,
                          &delay) == AHT20_OK);
  assert(AHT20_Initialize(&device) == AHT20_OK);
  assert(delay.total_ms == 50U);
  assert(fake.transmit_calls == 1U);
  assert(fake.transmit_lengths[0] == 3U);
  assert(fake.transmit_bytes[0][0] == 0xBEU);
  assert(fake.transmit_bytes[0][1] == 0x08U);
  assert(fake.transmit_bytes[0][2] == 0x00U);

  PushAhtMeasurement(&fake, 0x40000UL, 0x50000UL, 1U);
  assert(AHT20_Read(&device, &data) == AHT20_ERR_CRC);
  assert(!data.valid);

  fake.transmit_result = I2C_BUS_IO_ERROR;
  assert(AHT20_Read(&device, &data) == AHT20_ERR_WRITE_I2C);
  fake.transmit_result = I2C_BUS_OK;
  fake.receive_result = I2C_BUS_TIMEOUT;
  assert(AHT20_Initialize(&device) == AHT20_ERR_TIMEOUT);
}

static void test_aht20_instances_do_not_share_buses(void)
{
  FakeI2C fake_a;
  FakeI2C fake_b;
  I2C_Bus bus_a;
  I2C_Bus bus_b;
  FakeDelay delay_a = {0};
  FakeDelay delay_b = {0};
  AHT20_Device device_a;
  AHT20_Device device_b;
  AHT20_Data data_a;
  AHT20_Data data_b;

  FakeInit(&fake_a, 0x38U);
  FakeInit(&fake_b, 0x39U);
  bus_a = FakeMakeBus(&fake_a);
  bus_b = FakeMakeBus(&fake_b);
  assert(AHT20_DeviceInit(&device_a, &bus_a, 0x38U, FakeDelayMs,
                          &delay_a) == AHT20_OK);
  assert(AHT20_DeviceInit(&device_b, &bus_b, 0x39U, FakeDelayMs,
                          &delay_b) == AHT20_OK);

  PushAhtMeasurement(&fake_a, 0x80000UL, 0x60000UL, 0U);
  PushAhtMeasurement(&fake_b, 0x40000UL, 0x70000UL, 0U);
  assert(AHT20_Read(&device_a, &data_a) == AHT20_OK);
  assert(AHT20_Read(&device_b, &data_b) == AHT20_OK);
  AssertFloatNear(data_a.humidity_rh, 50.0f, 0.01f);
  AssertFloatNear(data_b.humidity_rh, 25.0f, 0.01f);
  AssertFloatNear(data_a.temperature_c, 25.0f, 0.01f);
  AssertFloatNear(data_b.temperature_c, 37.5f, 0.01f);
  assert(fake_a.transmit_calls == 1U);
  assert(fake_b.transmit_calls == 1U);
}

static void test_tmp117_requires_explicit_bus(void)
{
  TMP117_Device device;
  TMP117_Temperature temperature;
  I2C_Bus invalid_bus = {0};
  FakeI2C fake;
  I2C_Bus bus;

  memset(&device, 0, sizeof(device));
  FakeInit(&fake, TMP117_ADDR_GND);
  bus = FakeMakeBus(&fake);

  assert(TMP117_DeviceInit(NULL, &bus, TMP117_ADDR_GND) ==
         TMP117_ERR_PARAM);
  assert(TMP117_DeviceInit(&device, NULL, TMP117_ADDR_GND) ==
         TMP117_ERR_PARAM);
  assert(TMP117_DeviceInit(&device, &invalid_bus, TMP117_ADDR_GND) ==
         TMP117_ERR_PARAM);
  assert(TMP117_GetTemperature(&device, &temperature) == TMP117_ERR_PARAM);
  assert(TMP117_GetTemperature(NULL, &temperature) == TMP117_ERR_PARAM);
  assert(TMP117_GetTemperature(&device, NULL) == TMP117_ERR_PARAM);
}

static void test_tmp117_temperature_and_ready(void)
{
  FakeI2C fake;
  I2C_Bus bus;
  TMP117_Device device;
  TMP117_Temperature temperature;

  FakeInit(&fake, TMP117_ADDR_GND);
  bus = FakeMakeBus(&fake);
  assert(TMP117_DeviceInit(&device, &bus, TMP117_ADDR_GND) == TMP117_OK);

  FakePushU16(&fake, 0x1280U);
  assert(TMP117_GetTemperature(&device, &temperature) == TMP117_OK);
  assert(temperature.raw == 0x1280U);
  AssertFloatNear(temperature.temperature_c, 37.0f, 0.0001f);
  assert(fake.transmit_lengths[0] == 1U);
  assert(fake.transmit_bytes[0][0] == 0x00U);

  FakeInit(&fake, TMP117_ADDR_GND);
  bus = FakeMakeBus(&fake);
  assert(TMP117_DeviceInit(&device, &bus, TMP117_ADDR_GND) == TMP117_OK);
  FakePushU16(&fake, 0x0000U);
  assert(TMP117_GetReadyTemperature(&device, &temperature) ==
         TMP117_ERR_NOT_READY);

  FakeInit(&fake, TMP117_ADDR_GND);
  bus = FakeMakeBus(&fake);
  assert(TMP117_DeviceInit(&device, &bus, TMP117_ADDR_GND) == TMP117_OK);
  FakePushU16(&fake, 0x2000U);
  FakePushU16(&fake, 0xFB00U);
  assert(TMP117_GetReadyTemperature(&device, &temperature) == TMP117_OK);
  AssertFloatNear(temperature.temperature_c, -10.0f, 0.0001f);
}

static void test_tmp117_configuration_writes(void)
{
  FakeI2C fake;
  I2C_Bus bus;
  TMP117_Device device;

  FakeInit(&fake, TMP117_ADDR_VDD);
  bus = FakeMakeBus(&fake);
  assert(TMP117_DeviceInit(&device, &bus, TMP117_ADDR_VDD) == TMP117_OK);
  FakePushU16(&fake, 0x0000U);
  assert(TMP117_SetWorkMode(&device, TMP117_MODE_SHUTDOWN) == TMP117_OK);
  assert(fake.transmit_calls == 2U);
  assert(fake.transmit_lengths[1] == 3U);
  assert(fake.transmit_bytes[1][0] == 0x01U);
  assert(fake.transmit_bytes[1][1] == 0x04U);
  assert(fake.transmit_bytes[1][2] == 0x00U);

  FakeInit(&fake, TMP117_ADDR_VDD);
  bus = FakeMakeBus(&fake);
  assert(TMP117_DeviceInit(&device, &bus, TMP117_ADDR_VDD) == TMP117_OK);
  FakePushU16(&fake, 0x0000U);
  assert(TMP117_StartOneShot(&device, TMP117_AVERAGE_32) == TMP117_OK);
  assert(fake.transmit_calls == 2U);
  assert(fake.transmit_bytes[1][0] == 0x01U);
  assert(fake.transmit_bytes[1][1] == 0x0CU);
  assert(fake.transmit_bytes[1][2] == 0x40U);
  assert(TMP117_SetWorkMode(&device, (TMP117_Mode)2U) ==
         TMP117_ERR_RANGE);
  assert(TMP117_StartOneShot(&device, (TMP117_Averaging)4U) ==
         TMP117_ERR_RANGE);
}

static void test_tmp117_instances_do_not_share_buses(void)
{
  FakeI2C fake_a;
  FakeI2C fake_b;
  I2C_Bus bus_a;
  I2C_Bus bus_b;
  TMP117_Device device_a;
  TMP117_Device device_b;
  TMP117_Temperature temperature_a;
  TMP117_Temperature temperature_b;

  FakeInit(&fake_a, TMP117_ADDR_GND);
  FakeInit(&fake_b, TMP117_ADDR_VDD);
  bus_a = FakeMakeBus(&fake_a);
  bus_b = FakeMakeBus(&fake_b);
  assert(TMP117_DeviceInit(&device_a, &bus_a, TMP117_ADDR_GND) ==
         TMP117_OK);
  assert(TMP117_DeviceInit(&device_b, &bus_b, TMP117_ADDR_VDD) ==
         TMP117_OK);

  FakePushU16(&fake_a, 0x1000U);
  FakePushU16(&fake_b, 0x1400U);
  assert(TMP117_GetTemperature(&device_a, &temperature_a) == TMP117_OK);
  assert(TMP117_GetTemperature(&device_b, &temperature_b) == TMP117_OK);
  AssertFloatNear(temperature_a.temperature_c, 32.0f, 0.0001f);
  AssertFloatNear(temperature_b.temperature_c, 40.0f, 0.0001f);
  assert(fake_a.transmit_calls == 1U);
  assert(fake_b.transmit_calls == 1U);
}

int main(void)
{
  test_aht20_requires_explicit_dependencies();
  test_aht20_initializes_and_reads_known_values();
  test_aht20_calibration_and_errors();
  test_aht20_instances_do_not_share_buses();
  test_tmp117_requires_explicit_bus();
  test_tmp117_temperature_and_ready();
  test_tmp117_configuration_writes();
  test_tmp117_instances_do_not_share_buses();
  puts("sensor driver dependency-injection tests passed");
  return 0;
}
