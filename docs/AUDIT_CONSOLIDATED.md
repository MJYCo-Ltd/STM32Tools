# Consolidated reliability audit

## Scope and ownership

Common STM32 mechanisms remain in STM32Tools. Products choose UART/RTC handles,
clock restoration, task-health policy, board pin assignments and durable message
schemas. This change does not turn Auxiliary into Agriculture-specific code.

## Changes

- `MqttLineCollector` delegates to the common bounded line collector. On overflow
  or embedded NUL it discards the entire current line through LF; a valid-looking
  suffix cannot become an ACK. Its compatibility callback retains CR/LF.
- `ML307_MqttParseTextPublish` validates numeric ranges and actual fragment length.
  It is a text-line API, not an arbitrary binary/multiline MQTT framing engine.
  Complete business delivery also requires the expected connection and a complete
  payload. No declared-length equality is treated as actual-byte evidence.
- Auxiliary heap fields are byte-sized `size_t` values; MHz uses `uint32_t`.
  RTC/clock/debug UART are explicit borrowed dependencies. A wake-timer failure
  cannot enter STOP/Standby. Legacy void APIs retain the checked implementation;
  new users should inspect `AuxiliaryStatus`. `ReadFlash` remains a historical
  declaration without a library implementation; use the Flash APIs instead.
- `Button` has a bounded 16-edge FIFO per object and chronological debounce.
  Whole gestures survive an owner-task scheduling delay while capacity permits.
  Overflow is counted and suppresses uncertain input through a debounced release.
  GPIO/EXTI delivery and maximum edge rate still need target validation. A delayed
  process call may now emit the overdue 5s event before consuming its queued
  release; each gesture still emits at most one classified event.
- UART consumption has a per-port frame budget; a continuously replenished first
  port cannot monopolize `ProcessUart`. Each callback must itself be bounded.
  Existing DMA/queue-overwrite policy is not a claim of lossless reception at
  unlimited traffic rates. Hardware load/latency acceptance is still required.
- Pure `HealthMonitor` tracks required checkpoints, sticky deadline faults and
  a continuous stable interval. Callers provide serialization and monotonic time.
  It does not interpret offline networks as dead tasks and never feeds hardware.
- Firmware slots have an optional successful-step progress hook. Completed
  erases/CRC chunks are progress; hardware BUSY polls are not. Flash manifest and
  payload bytes are unchanged. The callback must not reenter the slot.
- The product may supply `BOOTLOADER_CONFIG_HEADER`. Legacy standalone F411
  defaults remain compatible, but are not the authority for Agriculture's map.
- Component targets expose source interfaces so App and Bootloader each compile
  with their own HAL/RTOS definitions. Generic sensor targets do not pull in HAL.

## Integration / compatibility

Rebuild all consumers: Button, STMSTATUS and StorageFirmwareSlot RAM layouts have
changed. Persistent layouts have not. Construct devices through their initializers.
Configure Auxiliary once before use. A missing RTC/clock binding now fails closed;
there is no implicit global `hrtc`, `SystemClock_Config`, or first-UART selection.
`IOInfo.h` owns I/O counters; Auxiliary retains compatibility type exposure.

AHT20/TMP117 remain context-based. `STM32Tools::aht20` and `::tmp117` depend on the
platform-neutral I2C bus; a board using HAL additionally links `::i2c_stm32`.
`STM32Tools::ml307` includes AT parser dependencies. The standalone MQTT collector
now also needs `Src/AT/ModuleFrameParser.c` linked.

## Verification

The complete source-archive host suite passed 19/19 under GCC Debug, GCC Release
and Clang ASan/UBSan before publication. Tests cover malformed lengths, collector
boundary loss, queued gestures/overflow, Auxiliary wake failures and heap width,
health faults, firmware milestones, and real bounded UART dispatch. The component
contract compiles/links the real generic sensor targets without any HAL headers.
GitHub Actions independently runs the complete clean checkout after publication.
Firmware timing, all STM32 families, RTOS scheduling under load and hardware
acceptance are not established by host tests. Low-power APIs do not implement a
whole-system STOP/tickless policy automatically.
