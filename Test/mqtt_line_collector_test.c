#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <AT/ModuleFrameParser.h>
#include <EWM103/ewm103_parser.h>

static unsigned header_parse_calls;
static ModuleFrameResult (*const real_parse_length_header)(
    const uint8_t *, size_t, const ModuleLengthFrameProtocolEx *, uint32_t *,
    size_t, ModuleFrameSpan *, const uint8_t **, size_t *) =
    ModuleFrameParser_ParseLengthHeader;
static ModuleFrameResult CountedParseLengthHeader(
    const uint8_t *data, size_t length, const ModuleLengthFrameProtocolEx *protocol,
    uint32_t *fields, size_t fields_capacity, ModuleFrameSpan *quoted,
    const uint8_t **payload, size_t *payload_length) {
  ++header_parse_calls;
  return real_parse_length_header(data, length, protocol, fields, fields_capacity,
                                  quoted, payload, payload_length);
}
#define ModuleFrameParser_ParseLengthHeader CountedParseLengthHeader
#include <Protocol/MqttLineCollector.h>
#undef ModuleFrameParser_ParseLengthHeader

/* Field values mirror production ML307_MqttReceiveProtocol in
 * Src/ML307/ml307_mqtt.c. The generic parser remains the only parser under test. */
static const ModuleLengthFrameProtocolEx ml307_protocol = {
  .base = {.prefix = "+MQTTURC: \"publish\",", .field_count = 5U,
    .payload_length_field = 4U, .field_separator = ',',
    .maximum_payload_length = 255U, .fixed_tail = "\r\n", .fixed_tail_length = 2U},
  .quoted_field = 3U, .maximum_quoted_length = 127U, .maximum_header_length = 176U
};
static const ModuleLengthFrameProtocolEx capacity_protocol = {
  .base = {.prefix = "+DATA:", .field_count = 1U, .payload_length_field = 0U,
    .field_separator = ',', .maximum_payload_length = 1024U,
    .fixed_tail = "\r\n", .fixed_tail_length = 2U}
};

typedef struct { unsigned calls; char last[MQTT_LINE_COLLECTOR_CAPACITY]; } Capture;
static void CaptureLine(char *line, void *ctx) {
  Capture *c = ctx; ++c->calls; strcpy(c->last, line);
}
static unsigned frames, text_lines;
static size_t text_bytes;
static uint8_t text_data[MQTT_LINE_COLLECTOR_CAPACITY];
static size_t text_line_start;
static char order[32];
static unsigned order_length;
static const ModuleLengthFrameProtocolEx *active_protocol;
static const uint8_t *expected_body;
static size_t expected_body_length;

static void frame(char *data, size_t length, void *context) {
  const uint8_t *body = NULL;
  size_t body_length = 0U, consumed = 0U;
  uint32_t fields[8];
  ModuleFrameSpan quoted;
  (void)context;
  if (active_protocol == &EWM103_MqttReceiveProtocol) {
    EWM103_MqttMessage message;
    assert(EWM103_ParseMqttReceive((const uint8_t *)data, length, &message) == MODULE_FRAME_COMPLETE);
    assert(message.link == 0U && message.topic.length == 3U && !memcmp(message.topic.data, "a,b", 3U));
    body = message.payload.data; body_length = message.payload.length;
  } else {
    assert(ModuleFrameParser_ParseLengthFrameEx((const uint8_t *)data, length,
      active_protocol, fields, 8U, &quoted, &body, &body_length, &consumed) == MODULE_FRAME_COMPLETE);
    assert(consumed == length);
    if (active_protocol == &ml307_protocol) {
      assert(fields[0] == 0U && fields[1] == 42U && fields[3] == body_length);
      assert(quoted.length == 3U && !memcmp(quoted.data, "a,b", 3U));
    }
  }
  assert(body_length == expected_body_length);
  if (body_length) assert(expected_body != NULL && !memcmp(body, expected_body, body_length));
  ++frames; order[order_length++] = 'F';
}
static void text(const uint8_t *data, size_t length, void *context) {
  (void)context;
  assert(length <= sizeof(text_data) - text_bytes);
  memcpy(text_data + text_bytes, data, length);
  text_bytes += length;
}
static void line(char *data, void *context) {
  size_t length = strlen(data);
  (void)context;
  assert(length > 0U && data[length - 1U] == '\n');
  assert(text_bytes - text_line_start == length);
  assert(!memcmp(text_data + text_line_start, data, length));
  text_line_start = text_bytes;
  ++text_lines; order[order_length++] = 'T';
}
static void reset(MqttLineCollector *c, const ModuleLengthFrameProtocolEx *protocol) {
  MqttLineCollector_Init(c, line, NULL);
  active_protocol = protocol;
  frames = text_lines = order_length = 0U; text_bytes = text_line_start = 0U;
  header_parse_calls = 0U; expected_body = NULL; expected_body_length = 0U;
}
static void feed(MqttLineCollector *c, const void *data, size_t length) {
  assert(length <= UINT16_MAX);
  MqttLineCollector_FeedFrames(c, data, (uint16_t)length, active_protocol, frame, text);
}
static size_t MakeEwmPacket(uint8_t *packet, size_t capacity,
                            const uint8_t *body, size_t body_length) {
  size_t header = (size_t)snprintf((char *)packet, capacity,
      "+MQTTSUBRECV:0,\"a,b\",%u,", (unsigned)body_length);
  assert(header + body_length + 2U <= capacity);
  if (body_length) memcpy(packet + header, body, body_length);
  memcpy(packet + header + body_length, "\r\n", 2U);
  return header + body_length + 2U;
}
static size_t MakeMlPacket(uint8_t *packet, size_t capacity,
                           const uint8_t *body, size_t body_length) {
  size_t header = (size_t)snprintf((char *)packet, capacity,
      "+MQTTURC: \"publish\",0,42,\"a,b\",%u,%u,",
      (unsigned)body_length, (unsigned)body_length);
  assert(header + body_length + 2U <= capacity);
  if (body_length) memcpy(packet + header, body, body_length);
  memcpy(packet + header + body_length, "\r\n", 2U);
  return header + body_length + 2U;
}

static void TestProtocol(const ModuleLengthFrameProtocolEx *protocol,
                         size_t (*make_packet)(uint8_t *, size_t, const uint8_t *, size_t)) {
  static const uint8_t binary_body[] =
    "\r\nOK\r\nERROR\r\nWIFI DISCONNECT\r\n+MQTTDISCONNECTED:0\r\n\0suffix";
  uint8_t packet[MQTT_LINE_COLLECTOR_CAPACITY];
  MqttLineCollector c;
  size_t length = make_packet(packet, sizeof(packet), binary_body, sizeof(binary_body) - 1U);
  size_t header = length - (sizeof(binary_body) - 1U) - 2U;
  reset(&c, protocol); expected_body = binary_body; expected_body_length = sizeof(binary_body) - 1U;
  feed(&c, packet, header);
  assert(!frames && !text_bytes && header_parse_calls > 0U);
  unsigned parses_after_header = header_parse_calls;
  feed(&c, packet + header, length - header);
  assert(frames == 1U && !text_bytes && !text_lines);
  assert(header_parse_calls == parses_after_header);

  for (size_t split = 0U; split <= length; ++split) {
    reset(&c, protocol); expected_body = binary_body; expected_body_length = sizeof(binary_body) - 1U;
    feed(&c, packet, split);
    if (split < length) {
      assert(!frames && !text_bytes && !text_lines);
      if (protocol == &EWM103_MqttReceiveProtocol) {
        EWM103_MqttMessage message;
        assert(EWM103_ParseMqttReceive(packet, split, &message) != MODULE_FRAME_COMPLETE);
      }
    }
    feed(&c, packet + split, length - split);
    assert(frames == 1U && !text_bytes && !text_lines);
  }

  length = make_packet(packet, sizeof(packet), NULL, 0U);
  reset(&c, protocol); expected_body = NULL; expected_body_length = 0U;
  feed(&c, packet, length); assert(frames == 1U && !text_bytes);
}

static void FramedTests(void) {
  uint8_t packet[MQTT_LINE_COLLECTOR_CAPACITY], large[800];
  MqttLineCollector c;
  TestProtocol(&EWM103_MqttReceiveProtocol, MakeEwmPacket);
  TestProtocol(&ml307_protocol, MakeMlPacket);

  /* Fixed-tail mismatch drains through LF, then a fresh valid frame recovers. */
  size_t n = (size_t)snprintf((char *)packet, sizeof(packet), "+MQTTSUBRECV:0,\"a,b\",3,abcXXignored\n");
  size_t good = MakeEwmPacket(large, sizeof(large), (const uint8_t *)"ok", 2U);
  reset(&c, &EWM103_MqttReceiveProtocol); expected_body = (const uint8_t *)"ok"; expected_body_length = 2U;
  feed(&c, packet, n); assert(!frames && !text_bytes); feed(&c, large, good);
  assert(frames == 1U && !text_bytes);

  /* Exercise the unchanged 512-byte acceptance edge with a permissive declaration. */
  size_t cap_header = (size_t)snprintf((char *)packet, sizeof(packet), "+DATA:%u,", 499U);
  size_t fit = sizeof(c.buffer) - 1U - cap_header - 2U;
  cap_header = (size_t)snprintf((char *)packet, sizeof(packet), "+DATA:%u,", (unsigned)fit);
  fit = sizeof(c.buffer) - 1U - cap_header - 2U;
  assert(fit + cap_header + 2U == sizeof(c.buffer) - 1U);
  memset(packet + cap_header, 'x', fit); memcpy(packet + cap_header + fit, "\r\n", 2U);
  reset(&c, &capacity_protocol); expected_body = packet + cap_header; expected_body_length = fit;
  feed(&c, packet, cap_header + fit + 2U); assert(frames == 1U && !text_bytes);
  n = (size_t)snprintf((char *)packet, sizeof(packet), "+DATA:%u,", (unsigned)(fit + 1U));
  memset(packet + n, 'x', fit + 1U); memcpy(packet + n + fit + 1U, "\r\n", 2U);
  reset(&c, &capacity_protocol); expected_body = packet + n; expected_body_length = fit + 1U;
  feed(&c, packet, n + fit + 3U); assert(!frames && !text_bytes);

  /* Oversize declarations drain by length and validate their fixed tail. */
  n = (size_t)snprintf((char *)large, sizeof(large), "+MQTTSUBRECV:0,\"a,b\",600,");
  for (size_t i = 0U; i < 600U; ++i) large[n + i] = (uint8_t)"\r\nOK\0ERROR"[i % 10U];
  memcpy(large + n + 600U, "\r\n", 2U);
  good = MakeEwmPacket(packet, sizeof(packet), (const uint8_t *)"ok", 2U);
  const size_t oversized_length = n + 602U;
  for (size_t split = 0U; split <= oversized_length; ++split) {
    reset(&c, &EWM103_MqttReceiveProtocol);
    feed(&c, large, split);
    assert(!frames && !text_bytes && !text_lines);
    feed(&c, large + split, oversized_length - split);
    assert(!frames && !text_bytes && !text_lines);
    expected_body = (const uint8_t *)"ok"; expected_body_length = 2U;
    feed(&c, packet, good);
    assert(frames == 1U && !text_bytes && !text_lines);
  }

  /* Malformed oversized tail resynchronizes without exposing discarded bytes. */
  n = (size_t)snprintf((char *)large, sizeof(large), "+MQTTSUBRECV:0,\"a,b\",600,");
  memset(large + n, 'x', 600U); memcpy(large + n + 600U, "X\n", 2U);
  reset(&c, &EWM103_MqttReceiveProtocol); feed(&c, large, n + 602U);
  assert(!frames && !text_bytes);

  /* Invalidate a partial frame, drain to the line boundary, then recover. */
  good = MakeEwmPacket(packet, sizeof(packet), (const uint8_t *)"abcdef", 6U);
  reset(&c, &EWM103_MqttReceiveProtocol); expected_body = (const uint8_t *)"abcdef"; expected_body_length = 6U;
  feed(&c, packet, good - 4U); assert(!frames); MqttLineCollector_Invalidate(&c);
  feed(&c, packet + good - 4U, 4U); assert(!frames);
  good = MakeEwmPacket(packet, sizeof(packet), (const uint8_t *)"ok", 2U);
  expected_body = (const uint8_t *)"ok"; expected_body_length = 2U; feed(&c, packet, good);
  assert(frames == 1U);

  /* Overlong, unterminated quoted topic is discarded through LF. */
  reset(&c, &EWM103_MqttReceiveProtocol);
  const char unterminated[] = "+MQTTSUBRECV:0,\"unterminated";
  feed(&c, unterminated, sizeof(unterminated) - 1U);
  memset(large, 'x', 200U);
  feed(&c, large, 200U);
  feed(&c, "\n", 1U);
  assert(!frames && !text_bytes && !text_lines);
  expected_body = (const uint8_t *)"ok"; expected_body_length = 2U;
  feed(&c, packet, good);
  assert(frames == 1U && !text_bytes && !text_lines);

  /* Unsigned overflow and malformed header are rejected; next frame recovers. */
  reset(&c, &EWM103_MqttReceiveProtocol);
  const char bad[] = "+MQTTSUBRECV:0,\"a,b\",4294967296,x\r\n";
  feed(&c, bad, sizeof(bad) - 1U); assert(!frames && !text_bytes);
  good = MakeEwmPacket(packet, sizeof(packet), (const uint8_t *)"ok", 2U);
  expected_body = (const uint8_t *)"ok"; expected_body_length = 2U; feed(&c, packet, good);
  assert(frames == 1U);

  /* Back-to-back frames and text preserve the early text-before-line order. */
  uint8_t pair[MQTT_LINE_COLLECTOR_CAPACITY];
  size_t first = MakeEwmPacket(pair, sizeof(pair), (const uint8_t *)"a", 1U);
  size_t second = MakeEwmPacket(pair + first, sizeof(pair) - first, (const uint8_t *)"b", 1U);
  reset(&c, &EWM103_MqttReceiveProtocol); expected_body = (const uint8_t *)"a"; expected_body_length = 1U;
  feed(&c, pair, first); expected_body = (const uint8_t *)"b";
  feed(&c, "OK\r\n", 4U); feed(&c, pair + first, second);
  assert(frames == 2U && text_lines == 1U && text_bytes == 4U && !memcmp(order, "FTF", 3U));

  /* Complete frames can also arrive in one call with no intervening text. */
  memcpy(pair + first, pair, first);
  reset(&c, &EWM103_MqttReceiveProtocol);
  expected_body = (const uint8_t *)"a"; expected_body_length = 1U;
  feed(&c, pair, first * 2U);
  assert(frames == 2U && !text_bytes && !text_lines && !memcmp(order, "FF", 2U));

  /* Existing numeric-only protocol initializers need no new fields. */
  const ModuleLengthFrameProtocol numeric = {
    .prefix = "+DATA:", .field_count = 2U, .payload_length_field = 1U,
    .field_separator = ',', .maximum_payload_length = 16U,
    .fixed_tail = "\r\n", .fixed_tail_length = 2U
  };
  const uint8_t raw[] = "+DATA:2,4,\0\nOK\r\n";
  uint32_t fields[2];
  const uint8_t *payload;
  size_t payload_length, consumed;
  assert(ModuleFrameParser_ParseLengthFrame(raw, sizeof(raw) - 1U, &numeric,
    fields, 2U, &payload, &payload_length, &consumed) == MODULE_FRAME_COMPLETE);
  assert(fields[0] == 2U && payload_length == 4U && consumed == sizeof(raw) - 1U);
  assert(!memcmp(payload, "\0\nOK", 4U));

  const uint8_t got_ip[] = "WIFI GOT IP\r\n";
  const uint8_t wifi_disconnect[] = "WIFI DISCONNECT\r\n";
  const uint8_t mqtt_disconnect[] = "+MQTTDISCONNECTED:0\r\n";
  assert(EWM103_ParseLinkEvent(got_ip, sizeof(got_ip) - 1U) == EWM103_LINK_EVENT_GOT_IP);
  assert(EWM103_ParseLinkEvent(wifi_disconnect, sizeof(wifi_disconnect) - 1U) ==
    EWM103_LINK_EVENT_DISCONNECTED);
  assert(EWM103_ParseLinkEvent(mqtt_disconnect, sizeof(mqtt_disconnect) - 1U) ==
    EWM103_LINK_EVENT_MQTT_DISCONNECTED);
}

int main(void) {
  FramedTests();
  MqttLineCollector c; Capture out = {0};
  MqttLineCollector_Init(&c, CaptureLine, &out);
  MqttLineCollector_Feed(&c, (const uint8_t *)"+MQTT", 5U); assert(out.calls == 0U);
  MqttLineCollector_Feed(&c, (const uint8_t *)":payload\r\n", 10U);
  assert(out.calls == 1U && strcmp(out.last, "+MQTT:payload\r\n") == 0);
  MqttLineCollector_Feed(&c, (const uint8_t *)"one\ntwo\n", 8U);
  assert(out.calls == 3U && strcmp(out.last, "two\n") == 0);
  uint8_t noise[MQTT_LINE_COLLECTOR_CAPACITY - 1U]; memset(noise, 'x', sizeof(noise));
  for (unsigned split = 0U; split < sizeof(noise); ++split) {
    out.calls = 0U; MqttLineCollector_Init(&c, CaptureLine, &out);
    MqttLineCollector_Feed(&c, noise, (uint16_t)split);
    MqttLineCollector_Feed(&c, noise + split, (uint16_t)(sizeof(noise) - split));
    const char *tail = "+MQTTURC: \"suback\",0,42,0\r\n";
    MqttLineCollector_Feed(&c, (const uint8_t *)tail, (uint16_t)strlen(tail));
    assert(out.calls == 0U);
    MqttLineCollector_Feed(&c, (const uint8_t *)"next\n", 5U);
    assert(out.calls == 1U && strcmp(out.last, "next\n") == 0);
  }
  out.calls = 0U; MqttLineCollector_Init(&c, CaptureLine, &out);
  const uint8_t nul_line[] = {'O', 'K', 0, 'x', '\n', 'a', '\n'};
  MqttLineCollector_Feed(&c, nul_line, sizeof(nul_line));
  assert(out.calls == 1U && strcmp(out.last, "a\n") == 0);
  MqttLineCollector_Invalidate(&c); MqttLineCollector_Feed(&c, (const uint8_t *)"tail\nnext\n", 10U);
  assert(out.calls == 2U && strcmp(out.last, "next\n") == 0);
  puts("MQTT collector binary framing, header parse cache, and legacy lines: passed");
  return 0;
}
