# Common STM32 mechanisms and explicit product contracts

## Auxiliary stays in STM32Tools

The library keeps zeroed allocation/free, heap diagnostics, STM32 UART debug
transmit, RTC wake-timer setup and Sleep/Stop/Standby mechanisms. A product
supplies `AuxiliaryConfig` at startup: its RTC handle, optional debug UART and
clock-restore callback. There is no implicit `GetUart(1)` or required `hrtc` /
`SystemClock_Config` symbol. No UART binding means debug output is disabled.
The existing implementation uses the F4-style HAL power API; other families
must provide a matching platform port, not assume identical low-power registers.

Use checked functions for error handling. A bad mode/counter/clock or failed
RTC deactivation/arming never enters low power. The caller owns wake IRQ setup,
RTOS tick suppression/elapsed-time correction, peripheral quiescence and IWDG
sleep budget. This is NOT a ready-made tickless implementation. Do not call it
arbitrarily from a live RTOS task. No RTC/LSE or modem reset pins are invented.

`STMSTATUS` now uses `size_t` for heap byte counts and `uint32_t` for CPU MHz.
`valid_fields` identifies measured fields; CPU load remains unmeasured. This is
an intentional native-struct ABI change: rebuild users; do not serialize this
structure. `IOInfo` is in `IOStatistics.h` (included by Auxiliary for compatibility).
The old unimplemented `ReadFlash` declaration is removed.

## UART ownership and bounded servicing

Configure the UART registry and loss callback before concurrent RX/service.
One task owns ProcessUart/ProcessUartBudget and services default ports
round-robin with an 8-chunk default budget. Ports registered through
`UartReceive_AddDedicated` are excluded from the global pump and pending check.
Their sole owner calls `UartReceive_ProcessPort`, optionally blocking on the
same RX queue with a timeout in kernel ticks. Callbacks do bounded work and
must avoid reentering receive service for the port. The owner also controls
stop and restart after startup. Stop disables autonomous restart, discards
the inbox and reports any pending loss in the owner's context.

Ports use normal ReceiveToIdle DMA with half-transfer interrupts disabled.
UART and RX DMA interrupts for a port use the same preemption priority. DMA
rearms on the alternate buffer before submitting the completed queue item.
The owner copies the next item into stable callback storage. Registry lifetime
is the application lifetime.

On queue overflow, malformed size or HAL receive error, subsequent ISR chunks
are discarded until the consumer flushes the old inbox and reports loss. The
loss callback runs in owner context before post-gap bytes are decoded. Loss
wakes a blocked dedicated owner even when no data arrived. Its
counter counts observed dropped/error chunks, not exact missing byte counts.
Protocols must invalidate partial AT/MQTT/HTTP/RS485 state; continuing a broken
stream as a complete record is unsafe. Bare-metal one-slot inboxes follow the
same fail-closed policy. Read the 64-bit legacy IO counters only with suitable
synchronization if an atomic snapshot is required on a 32-bit MCU.

## Button history

Each button retains 16 timestamped edges by default. IRQ critical sections do
not delay or call business callbacks. The task replays stable intervals in
order, then samples GPIO at a due deadline. One Process call returns at most
one event; NextWakeDelay=0 means more work remains. This tolerates finite task
delay, not unlimited bursts. Overflow drops the incomplete gesture, exposes a
counter and resynchronizes; it cannot invent a destructive long-press action.
A hold crossing 5 s can be emitted immediately when a delayed task observes a
later release. The release does not emit that event again. Stable idle is not
polled. Object lifetime, unique EXTI lines and task ownership still apply.

## MQTT stream boundaries

`MqttLineCollector_Feed` adapts the shared `ModuleLineCollector` and preserves
its legacy CRLF callback contract. Oversize or embedded-NUL text records are
discarded through LF. Callbacks must not recursively feed/reset the same
collector.

`MqttLineCollector_FeedFrames` consumes each module's declared length-frame
protocol. Once the generic header parser validates a frame header, the collector
tracks the declared body and fixed tail without reparsing body bytes. The body
is opaque binary data: NUL, CR/LF, `OK`/`ERROR`, and URC-like bytes stay out of
the AT text and line callbacks. Frames that exceed the existing collector or
protocol size limits are drained by their declared length and are not delivered.

The ML307 text PUBLISH parser separately checks connection/message IDs,
unsigned lengths, actual fragment byte count and fragment <= total; it preserves
payload spaces. The product accepts only complete payloads for its selected
connection. Never reinterpret a partial fragment as a business message. HTTP
binary readers remain separate.

## HealthMonitor

The allocation-free core is HAL/RTOS independent. The caller serializes access
and supplies monotonic uint32 milliseconds, task IDs and deadlines. Two observed
progress points plus a healthy interval may authorize a product milestone.
Missed deadlines latch; late heartbeats do not heal a stalled task. Finite
operation leases cannot be renewed while active, and do not excuse other tasks.
An offline network or a failed peripheral reading is not, by itself, a stalled
state machine. Hardware refresh ownership and OTA confirmation policy belong
to the product.

## Named CMake components

Include the top-level library with add_subdirectory and link STM32Tools::Core,
AT, Storage, SignedFirmware, ST7305, PlatformSTM32F4, IwdgSTM32F4,
BootloaderSTM32F4, I2CBus, AHT20, TMP117, W25Q, Modbus, HttpRange, ML307,
EWM103, Utilities or HealthMonitor as needed. Components expose sources as
INTERFACE usage requirements so App RTOS objects are never accidentally reused
in the bare-metal Bootloader. The Modbus queue/RS485 core and platform component
still require their documented CMSIS/FreeRTOS or HAL environment. The portable
component-contract host test deliberately links without either environment.
