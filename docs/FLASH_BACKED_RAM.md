# Paged display and bounded log streaming

## Scope and compatibility

This change adds reusable mechanisms only. Product Flash partitions, RTOS heap
sizes, DMA ownership, signing policy and UI state remain the caller's concern.
It does not map SPI NOR Flash into the CPU's RAM address space.

All users must rebuild: `ST7305_Binding` has an appended `page_rows` field.
A zero-initialized/omitted field keeps the legacy full-frame path.

## ST7305 page rendering

Set `page_rows` to a non-zero even count not exceeding native panel height.
Provide `ceil(native_width / 8) * page_rows` framebuffer bytes and the usual
line-transfer buffer. For the 300 x 400 profile, forty native rows need 1,520
bytes instead of 15,200 bytes. Page coordinates are independent of rotation.

Call `LCD_ST7305_RenderPaged(callback, context)`. The driver clears a page,
replays the callback, and sends only those native rows. The callback must draw
the SAME frozen scene for the entire call; it must not mutate application state,
change rotation, perform control actions, or start another frame. Any callback
or transfer failure invalidates ready state; explicitly reinitialize then redraw.
Large filled primitives are clipped before pixel iteration. A draw outside an
active paged callback is ignored. Legacy Refresh/RefreshArea are rejected in
paged mode; those APIs retain their existing behavior in full-frame mode.

Initialization sends a complete blank frame before the driver becomes ready.
The first implementation repaints the whole panel on every RenderPaged call;
it is not a dirty-page scheduler. Measure refresh latency on the real board.

`st7305_paged_test` compares the transmitted pixel bytes of full-frame and
40-row rendering in all four rotations, checks buffer canaries, rejects nested
rendering and invalid bindings, and tests transfer failure/reinitialization.

## Log cursor and per-record acknowledgement

Zero-initialize `StorageLogCursor`, then repeatedly call
`StorageLog_NextPending`. BUSY means the bounded scan made progress, NOT_FOUND
ends one pass, and OK returns one CRC-validated payload plus a receipt. At most
eight record candidates and one sector boundary are serviced per invocation.
The product may apply an idle backoff after a completed pass. Do not allocate an
array proportional to the number of retained Flash log entries.

Hold onto that one payload and receipt until the product's delivery-completion
policy succeeds. Call `StorageLog_Acknowledge` only after that completion; it
checks sector generation, record sequence, payload length and CRC before
programming the existing commit word to zero. ACK does not erase a sector.
A retry with the same receipt is idempotent. A failed verification/read must not
be treated as delivery success. A partial ACK is replayed after reset; an
incomplete original commit is not delivered. Duplicate delivery remains possible
and no new application-level broker acknowledgement protocol is introduced.

### Storage lifecycle migration

Record headers and payload bytes remain V1 and existing pending records remain
readable, but zero commit words now mean durable ACK tombstones. This is a
**lifecycle extension**, not a claim that every older reader understands them.
The new Init accounts for tombstones when reconstructing the write cursor and
sequence. Clear retains a new sector generation to invalidate stale receipts.

Rebuild and qualify every shared writer, including a Bootloader, with this
implementation before enabling per-record ACK in a product. Do not silently mix
this lifecycle with an old Bootloader or unqualified rollback firmware. Normal
bounded-ring overwrite policy remains unchanged; this is not unlimited retention.
Serialize log APIs through the existing product Flash mutex, and reset active
cursors when deliberately clearing/reinitializing a log.

`storage_log_stream_test` covers more than 24 records, append during delivery,
no erase during ACK, reboot reconstruction, partial ACK and partial original
commit, read faults, stale receipts after sector reuse and explicit Clear.

## Validation

Native suites run with assertions enabled in Release as well as Debug. Added
cases are included in the normal Test/CMakeLists.txt entry point. GCC Debug,
GCC Release and Clang ASan/UBSan passed 28 tests in the implementation workspace.
These are host tests, not physical display/Flash/OTA qualification.
