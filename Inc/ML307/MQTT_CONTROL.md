# ML307 MQTT control decoding

## Scope

The strict grammar for `conn`, `suback`, `puback`, `timeout`, `+MQTTSUB`,
`+MQTTPUB` and `+MQTTSTATE` is now in `ml307_mqtt.c`. It uses the existing
`AT/at_codec` line primitives and `ModuleFrameParser_ParseUnsigned` rather than
an Agriculture-specific second parser. Link BOTH `Src/AT/at_codec.c` and
`Src/AT/ModuleFrameParser.c` when compiling the MQTT helper.

This migration preserves the existing Agriculture control grammar and status
semantics. It does not claim support for a new modem firmware/manual revision.
The command builders, text PUBLISH path, and other legacy URC kinds are not
redesigned here. Binary payloads, fragmentation, collector overflow handling,
UART backpressure and session authentication are separate work.

## Two different input contracts

`ML307_MqttParseControlUrc(data, length, &event)` consumes **one complete line**.
A collector must establish that boundary first. Its slice need not have a NUL
terminator; length excludes any NUL. Terminal CR/LF is optional, embedded NUL is
rejected, and failures clear the event. This is not a stream parser: a partial
numeric token can itself look valid, so it must not be called on arbitrary DMA
fragments and treated as a frame-completion detector.

The `ML307_MqttParse*Response` functions scan **bounded command buffers**. Only
LF-terminated lines qualify (an unterminated trailing line cannot supply a
message ID or connection state). Command metadata uses the first matching line;
a malformed matching line is not skipped in favor of a later duplicate.
Connection lookup retains the last valid matching complete line, supporting
coalesced connecting/connected notifications and ignoring other connection IDs.
Outputs change only on success.

The legacy `ML307_MqttParseUrc(const char *, ...)` still accepts a NUL-terminated
line/response. It delegates the four control kinds to the same strict grammar;
a malformed control record never falls through to permissive sscanf parsing.
It selects at a line boundary, not a URC-looking substring in unrelated text.
Its PUBLISH/other legacy branches are NOT a bounded binary API.

## Adapter responsibilities

After successful decoding, correlate event type, connection ID and message ID
with the active command. An AT `OK` is not a broker acknowledgment. A broker ACK
is not the application's database/persistence receipt. Do not discard durable
records based only on either of those transport signals.

The product owns command tokens, session generation, retries, early ACK state,
connection/download ownership and business transitions. The library owns field
arity, numeric overflow/range checking and control status validity. Unknown or
malformed control states cannot become success by defaulting a missing field.

## Tests

`Test/ml307_mqtt_control_test.c` is registered in the normal CTest entry. It
covers exact field counts, ranges, status codes, LF boundaries, coalesced states,
unchanged outputs after failure, legacy forwarding, exact-size non-NUL slices,
and 10,000 deterministic noise inputs under the sanitizer configuration.
