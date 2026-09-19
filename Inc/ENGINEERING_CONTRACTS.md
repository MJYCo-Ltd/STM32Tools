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
One task owns ProcessUart/ProcessUartBudget. The default budget is 8 chunks;
ports are serviced round-robin. Receive callbacks must themselves do bounded
work. Ports use normal (not circular) ReceiveToIdle DMA, with half-transfer
interrupts disabled. StopReceive prevents autonomous restart. Registry lifetime
is the application lifetime; dynamically removing active UARTs is unsupported.

On queue overflow, malformed size or HAL receive error, subsequent ISR chunks
are discarded until the consumer flushes the old inbox and reports loss. The
loss callback runs in task context before post-gap bytes are decoded. Its
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

MqttLineCollector now adapts the shared ModuleLineCollector and preserves its
legacy CRLF callback contract. Oversize or embedded-NUL records are discarded
through LF. Callbacks must not recursively feed/reset the same collector.
The ML307 text PUBLISH parser checks connection/message IDs, unsigned lengths,
actual fragment byte count and fragment <= total; it preserves payload spaces.
The product accepts only complete payloads for its selected connection.
This is a bounded TEXT interface. Binary/newline-bearing MQTT payloads and
fragment reassembly are not advertised as supported. Never reinterpret a
partial fragment as a business message. HTTP binary readers remain separate.

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
