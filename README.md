## 仓库说明

- 代码基于STM32CubeMX生成的工程
- main.c 是如何调用32Modules中代码的一个示例
# FreeRTOS的注意事项
- FreeRTOS每个任务都有栈空间，空间超过设定大小就会栈溢出，导致系统崩溃，我把其他代码都让AI检查了，最后AI提示注意栈空间问题——不要在任务中定义数组这种占用栈空间的代码。
- 使用pvMalloc需要在开启任务循环后才能进行，如果直接在系统初始化之后，结果未可知。最好在一个任务进入任务循环之前进行空间开辟
# 串口通信
## cubemx中的设置
- 在DMA Settings 选项下增加 USART_RX
- 在NVIC Settings 选项下开启 global interrupt
- 开辟一个新的Task用于处理 ProcessUart();如果在DefaultTask中调用，会因为执行其他代码而阻塞
- 如果启用了BeginReceiveUartInfo，将一直调用 HAL_UARTEx_ReceiveToIdle_DMA，RxState 将一直处于 HAL_UART_STATE_BUSY_RX状态
## 性能
- 经过测试在目前 每个串口有100个字节的缓存区的情况下，每50毫秒，接收51个字节，不会丢失数据

# UC8253 支持
增加了对8253的支持，可以全刷和局刷
示例代码如下，
```cEPD_Init(EPD_THREE_COLOR, 1);
    EPD_PowerOn();
    EPD_Clear(EPD_WHITE);
    EPD_Update();

    EPD_InitDrawBuffer(EPD_WHITE);
    EPD_DrawRect(10, 10, 100, 60, EPD_BLACK);
    EPD_DrawFilledRect(130, 10, 50, 50, EPD_BLACK);
    EPD_DrawCircle(120, 200, 40, EPD_BLACK);
    EPD_DrawFilledCircle(60, 200, 25, EPD_BLACK);
    EPD_DrawLine(0, 0, 239, 415, EPD_BLACK);
    EPD_DrawString(0, 300, "##$$ !$#", EPD_BLACK);
    EPD_ShowBuffer();
    EPD_Update();

    for (uint8_t index = 0; index < 10; ++index) {
      EPD_InitDrawBuffer(EPD_BLACK);
      EPD_DrawFilledCircle(40 + index * 10, 104 + index * 10, 10, EPD_WHITE);
      EPD_DisplayPartial(40, 104, 120, 216);
      osDelay(2000);
    }

    EPD_PowerOff();
    EPD_DeepSleep(); 
```
# 爱氪森传感器 支持
- 解析爱氪森传感器数据用于冷库数据监控
# EEPROM 读写支持
- 如果使用 字节模式，就要一直使用字节模式，切换模式需要先擦除整个EEPROM，以防因为字节未对齐导致的不可控行为
- 在HAL 初始化完成后 需要调用一下 srand(HAL_GetTick())，可以随机的从EEPROM中有效地址开始写数据