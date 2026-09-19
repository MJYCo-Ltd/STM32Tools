# Reusable I2C sensor drivers

`AHT20` and `TMP117` are device drivers, not board services. They do not include
`main.h`, select `hi2c1`, or choose an RTOS/HAL delay implementation.

## Dependency ownership

The product/board layer owns:

- the concrete STM32 I2C handle;
- construction of the `I2C_Bus` port;
- delay/scheduling policy;
- sensor address selection;
- application validity/range rules.

The driver owns only the sensor protocol, register access, CRC and physical-unit
conversion.

## AHT20

```c
static I2C_Bus bus;
static AHT20_Device sensor;

static void DelayMs(void *context, uint32_t delay_ms)
{
    (void)context;
    (void)osDelay(delay_ms);
}

I2C_BusInitSTM32(&bus, &hi2c1, 100U);
AHT20_DeviceInit(&sensor, &bus, AHT20_I2C_ADDR7, DelayMs, NULL);
AHT20_Initialize(&sensor);

AHT20_Data data;
AHT20_Status status = AHT20_Read(&sensor, &data);
```

Each `AHT20_Device` retains its own bus, address and delay callback. Multiple
instances do not share hidden global state.

## TMP117

```c
static I2C_Bus bus;
static TMP117_Device sensor;

I2C_BusInitSTM32(&bus, &hi2c1, 100U);
TMP117_DeviceInit(&sensor, &bus, TMP117_ADDR_GND);

TMP117_Temperature temperature;
TMP117_Status status = TMP117_GetTemperature(&sensor, &temperature);
```

`TMP117_Temperature` contains only the raw register value and the converted
Celsius value. Medical/body-temperature ranges are product policy and must be
checked by the calling application.

## Testing

`Test/sensor_driver_test.c` uses two independent scripted buses and covers:

- missing/invalid dependencies;
- I2C write, read and timeout errors;
- AHT20 calibration, CRC and known-value conversion;
- TMP117 data-ready, mode and one-shot configuration;
- negative and positive temperature conversion;
- multiple devices without shared global state.
