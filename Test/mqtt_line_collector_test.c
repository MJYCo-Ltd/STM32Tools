#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <Protocol/MqttLineCollector.h>

typedef struct {
  unsigned int calls;
  char last_line[MQTT_LINE_COLLECTOR_CAPACITY];
} Capture;

static void capture_line(char *line, void *context)
{
  Capture *capture = (Capture *)context;
  ++capture->calls;
  (void)strncpy(capture->last_line, line, sizeof(capture->last_line) - 1U);
  capture->last_line[sizeof(capture->last_line) - 1U] = '\0';
}

static void test_fragmented_line(void)
{
  MqttLineCollector collector;
  Capture capture = {0U, {0}};
  const uint8_t first[] = "+MQTT";
  const uint8_t second[] = ":payload\r\n";

  MqttLineCollector_Init(&collector, capture_line, &capture);
  MqttLineCollector_Feed(&collector, first, (uint16_t)(sizeof(first) - 1U));
  assert(capture.calls == 0U);
  MqttLineCollector_Feed(&collector, second, (uint16_t)(sizeof(second) - 1U));
  assert(capture.calls == 1U);
  assert(strcmp(capture.last_line, "+MQTT:payload\r\n") == 0);
}

static void test_multiple_lines(void)
{
  MqttLineCollector collector;
  Capture capture = {0U, {0}};
  const uint8_t data[] = "one\ntwo\n";

  MqttLineCollector_Init(&collector, capture_line, &capture);
  MqttLineCollector_Feed(&collector, data, (uint16_t)(sizeof(data) - 1U));
  assert(capture.calls == 2U);
  assert(strcmp(capture.last_line, "two\n") == 0);
}

static void test_overflow_recovers(void)
{
  MqttLineCollector collector;
  Capture capture = {0U, {0}};
  uint8_t data[MQTT_LINE_COLLECTOR_CAPACITY + 4U];

  (void)memset(data, 'x', sizeof(data));
  data[sizeof(data) - 1U] = '\n';
  MqttLineCollector_Init(&collector, capture_line, &capture);
  MqttLineCollector_Feed(&collector, data, (uint16_t)sizeof(data));
  assert(capture.calls == 0U); /* Discard the entire overlong record, including its tail. */
  MqttLineCollector_Feed(&collector, (const uint8_t *)"next\r\n", 6U);
  assert(capture.calls == 1U);
  assert(strcmp(capture.last_line, "next\r\n") == 0);
}

int main(void)
{
  test_fragmented_line();
  test_multiple_lines();
  test_overflow_recovers();
  return 0;
}
