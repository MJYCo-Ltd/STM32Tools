# Branch integration — 2026-09-19

## Recorded inputs

| Branch | Input commit |
| --- | --- |
| master / fix/integrated-reliability | a39693e408cb069beb6cb73d0a6d8ca662426413 |
| fix/consolidated-runtime-boundaries | 300249ddc929da3ba7ac64f90c39a0862104dd7f |
| fix/consolidated-engineering | 7532b6c8af3a27a44e1469974714e708f701283e |
| fix/audit-consolidated | 06cc5e4e400aa405d55a6816dc428cf1d6b8c5c3 |
| refactor/batch3-mqtt-control (already an ancestor) | 6d40f8fd906c641a14079f6bfb2fc518ec6f2f98 |

Integration preserves these histories and leaves their branches and master
untouched. The engineering and audit branches had 20 conflicting files; neither
whole-tree overwrite nor an 'ours' history-only merge was used.

## Resolutions

- Keep engineering IRQ-safe button lifecycle/history and overflow recovery; add
  audit suppression of uncertain gestures until a debounced release. Retain both
  queued-edge and chronological-history regressions.
- Keep engineering global round-robin UART service, loss notification in task
  context before post-gap delivery, fail-closed inbox handling and bare-metal
  coverage. Retain audit's configurable `UART_RECEIVE_PROCESS_BUDGET` name,
  defined globally (default 8). Tests also compile it as 2. A product must route
  loss to its protocol state machines; no implicit product callback is added.
- Use one bounded line collector; preserve normal/raw entry points and CRLF
  compatibility. Oversize/NUL lines discard through LF. Both ML307 text-publish
  parser names call the same strict parser; binary/multiline payloads and fragment
  reassembly are not introduced.
- Keep checked, explicitly configured Auxiliary dependencies; preserve audit
  status aliases and last-error diagnostic. No implicit debug UART/RTC/clock.
  The low-power methods are mechanisms, not automatic RTOS tickless idle.
- Keep one `System/HealthMonitor.h` API and implementation. Port audit regression
  semantics onto it rather than retain two incompatible types/include guards.
  Hardware watchdog policy remains a product responsibility.
- Retain successful erase/CRC progress hooks from audit. RAM struct layouts change
  (Button, STMSTATUS, StorageFirmwareSlot); rebuild and use initializers. Existing
  on-flash layouts are unchanged.
- Require a product Bootloader map. Accept either `STM32TOOLS_BOOT_CONFIG_HEADER`
  or audit's `BOOTLOADER_CONFIG_HEADER`, but not both. Old F411 linker material is
  an example, not a default product configuration.
- Keep engineering named source-interface components and add audit lowercase
  aliases/fine-grained platform components. IOInfo has one authoritative type.
- Replace duplicate/one-off CI publication and snapshot jobs with one native
  validation matrix: GCC Debug, GCC Release, Clang ASan/UBSan. All registered
  tests remain enabled, including assert-based checks in Release.

## Verification boundaries

The integrated suite has 26 registered host tests. Consult the exact revision's
CI evidence for execution results. Host tests cannot prove real EXTI/DMA timing,
interrupt priorities, stack/heap margins, Flash power-cut behavior or low-power
wake performance. No target deployment or production-signing claim is made.

The separate private agricultural product owns its board configuration, UART
loss routing and exact dependency lock. No product source or signing key is
introduced into this public library.
