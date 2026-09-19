# Compile-checked public API examples

`api_binding_example.c` demonstrates the context-based AHT20/TMP117 APIs,
dual-edge Button processing/deadline API, and checked ST7305 binding/initialization.
It is compiled as an OBJECT target by the main `Test` CMake entry, with the real
public headers and host GPIO type declarations. It is not linked as a firmware,
does not implement a board, and does not pretend to exercise hardware.

The caller supplies valid output pointers and long-lived devices/buses/buffers.
Configure the GPIO for BOTH rising and falling EXTI before initializing Button,
wake its owner on either edge, and process both edge notifications and deadlines.
Never interpret a returned millisecond delay as RTOS ticks without conversion.
Keep board pins, peripheral handles and scheduler policy in the product.

`api_contract_test` additionally uses C11 `_Generic` to check public signatures.
Driver behavior remains covered by the separate button/sensor/display tests.
