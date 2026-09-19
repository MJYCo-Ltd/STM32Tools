# Reusable I2C sensor drivers

AHT20 and TMP117 use explicit per-device contexts. Neither selects `hi2c1`,
includes the product `main.h`, or chooses an RTOS scheduler internally.

The product owns the concrete I2C peripheral, `I2C_Bus` port, delay callback,
bus locking, lifetime and application validity rules. The driver owns commands,
register access, CRC and physical-unit conversion. Multiple contexts do not make
concurrent access to a shared bus automatically safe.

## Current API (breaking migration from the old global bus API)

```c
static I2C_Bus bus;
static AHT20_Device aht;
static TMP117_Device tmp;

/* Implement in the product. The argument is milliseconds, NOT RTOS ticks.
 * An RTOS implementation converts to ticks, rounding up as needed. */
extern void BoardDelayMs(void *context, uint32_t milliseconds);

/* In product initialization, after peripheral and scheduler setup: */
I2C_BusInitSTM32(&bus, &hi2c1, 100U);
AHT20_Status aht_status = AHT20_DeviceInit(
    &aht, &bus, AHT20_I2C_ADDR7, BoardDelayMs, NULL);
if (aht_status == AHT20_OK) aht_status = AHT20_Initialize(&aht);
/* Caller must report or handle a non-OK status. */

TMP117_Status tmp_status = TMP117_DeviceInit(&tmp, &bus, TMP117_ADDR_GND);
/* Caller must report or handle a non-OK status. */

AHT20_Data sample;
if (aht_status == AHT20_OK) aht_status = AHT20_Read(&aht, &sample);
TMP117_Temperature temperature;
if (tmp_status == TMP117_OK) tmp_status = TMP117_GetTemperature(&tmp, &temperature);
```

The above is a product integration fragment, not a standalone C translation
unit. Compiled functions using the same public APIs are in
[`Examples/ApiUsage/api_binding_example.c`](../Examples/ApiUsage/api_binding_example.c).

AHT20 has a fixed `0x38` address. Multiple physical AHT20 devices need separate
buses or appropriate multiplexing. TMP117 address selection follows its ADD0
wiring. Borrowed buses and delay contexts must outlive all sensor operations.

`TMP117_Temperature` contains `raw` and `temperature_c`; sensor success is not
proof that a temperature is a valid body-temperature measurement. Product-specific
measurement and alarm rules stay outside STM32Tools.

## Verification

`Test/sensor_driver_test.c` covers scripted-bus behavior. The compile-only usage
example checks API integration, and `api_contract_test` checks signatures and
constants without running a physical device. None substitutes for I2C electrical,
concurrency or on-board validation. See [`Test/README.md`](../Test/README.md).
