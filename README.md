# STM32Tools 仓库说明

- 代码基于 STM32CubeMX 生成的工程
- `main.c` 为调用各模块的示例

## 目录结构

```
STM32Tools/
├── Inc/                    # 头文件及用户配置
│   ├── Display/            # 显示相关
│   │   ├── SPIDisplay.h    # SPI 显示统一接口（EPD/LCD 共用）
│   │   ├── LCD/            # ST7789 配置
│   │   └── EPD/            # UC8253 墨水屏配置
│   ├── Camera/             # 摄像头驱动
│   │   ├── ov5640.h        # OV5640 500 万像素
│   │   └── ov2640.h        # OV2640 200 万像素
│   ├── TMP/                # TMP117 温度传感器
│   ├── MX/                 # MX-22 蓝牙模块
│   ├── L051C8T6/           # STM32L051 专用（EEPROM）
│   ├── pb/                 # nanopb 协议缓冲
│   ├── MQTT/               # MQTT 客户端
│   ├── Tracealzer/         # Percepio Tracealyzer 追踪
│   ├── QSPIFlash.h         # QSPI Flash (W25Q) 接口
│   └── ...
├── Src/                    # 源文件
│   ├── Display/            # LCD、EPD、Graphics 实现
│   ├── Camera/             # OV5640、OV2640 驱动
│   ├── TMP/                # TMP117 驱动
│   ├── MX/                 # MX-22 驱动
│   ├── pb/                 # nanopb 编解码
│   ├── MQTT/               # MQTT 核心
│   ├── Tracealzer/         # Tracealyzer 运行时
│   └── ...
└── Test/                   # 单元测试
```

## FreeRTOS 注意事项

- 每个任务都有栈空间，超过设定大小会栈溢出导致系统崩溃
- **不要在任务中定义大数组**等占用栈空间的变量
- 使用 `pvPortMalloc` 需在任务进入循环后进行，系统初始化后立即调用结果未可知

## 串口通信 (UartReceive)

### CubeMX 设置

- DMA Settings：增加 USART_RX
- NVIC Settings：开启 global interrupt
- 需单独开辟 Task 调用 `ProcessUart()`，在 DefaultTask 中调用会因其他代码阻塞

### 注意事项

- 启用 `BeginReceiveUartInfo` 后，会持续调用 `HAL_UARTEx_ReceiveToIdle_DMA`，RxState 将保持 `HAL_UART_STATE_BUSY_RX`

### 性能

- 每串口 100 字节缓冲区下，每 50ms 接收 51 字节，测试无丢包

## 显示模块

### SPI 显示统一接口 (SPIDisplay.h)

EPD 与 LCD 共用 `SPI_SendCommand`、`SPI_SendData`、`SPI_SendBuffer`。头文件为内联实现，需在包含前由用户配置定义以下宏：

| 宏 | 说明 |
|----|------|
| `SPI_SELECT` | 片选有效 |
| `SPI_UNSELECT` | 片选释放 |
| `SPI_SEND_CMD` | DC 置为指令模式（低电平） |
| `SPI_SEND_DATA` | DC 置为数据模式（高电平） |
| `DISPLAY_SPI_PORT` | SPI 句柄，如 `hspi1` |
| `USE_BUFFER` | 可选，大块数据使用 DMA 传输 |

### 绘图接口 (Graphics)

基于 `DrawPixel` 的通用绘图库，LCD 与 EPD 共用同一套接口。底层由各自实现 `DrawPixel` 区分。

- **直线**：Bresenham 算法
- **圆/填充圆**：中点圆算法
- **三角形**：边缘追踪扫描填充

`DrawHLine(x0, y0, x1, color)` 与 `DrawVLine(x0, y0, y1, color)` 的第三个参数为**终点坐标**，非长度。

### LCD (ST7789)

- 支持 135x240、240x240、170x320 分辨率
- 在 `Inc/Display/LCD/lcd_user.c` 中配置 SPI 端口、引脚、分辨率
- 包含 `lcd_user.c` 后包含 `SPIDisplay.h`，即可使用统一 SPI 接口

### EPD 墨水屏 (UC8253)

- 支持双色/三色模式，全刷与局刷
- 在 `Inc/Display/EPD/epd_user.c` 中配置 SPI、GPIO、引脚映射
- `epd_uc8253.c` 实现 `DrawPixel`，可直接使用 Graphics 接口
- 包含 `epd_user.c` 后包含 `SPIDisplay.h`，即可使用统一 SPI 接口

示例代码：

```c
EPD_Init(EPD_THREE_COLOR, 1);
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

// 局刷示例
for (uint8_t index = 0; index < 10; ++index) {
  EPD_InitDrawBuffer(EPD_BLACK);
  EPD_DrawFilledCircle(40 + index * 10, 104 + index * 10, 10, EPD_WHITE);
  EPD_DisplayPartial(40, 104, 120, 216);
  osDelay(2000);
}

EPD_PowerOff();
EPD_DeepSleep();
```

> 注：`EPD_DrawRect`、`EPD_WHITE`、`EPD_BLACK` 等由项目中的 `epd_graphics.h` 提供，需在工程中引入对应头文件并实现颜色常量与绘图接口的 EPD 封装。

## 爱氪森传感器 (ECSense)

- 解析爱氪森传感器数据，用于冷库等环境监控
- 支持多种气体类型（甲醛、VOC、CO2 等）

## EEPROM 读写 (L051C8T6)

- 支持 STM32L051 内置 EEPROM
- **字节模式**：一旦使用字节模式，需一直使用，切换模式前需先擦除整个 EEPROM
- 建议在 HAL 初始化后调用 `srand(HAL_GetTick())`，用于随机选择有效写入地址

## 摄像头 (Camera)

### OV5640

- 500 万像素，I2C/SCCB 配置寄存器，DCMI 接收图像
- 支持分辨率：160x120、320x240、480x272、640x480、800x480
- 支持格式：RGB565、RGB888、YUV422、Y8、JPEG
- 需实现 `OV5640_IO_t` 并注册 I2C 读写回调，详见 `Inc/README.md` 及 `ov5640_user.h`

### OV2640

- 200 万像素，I2C 配置，DCMI 接收
- 需提供 `camera.h` 等 BSP 依赖

## QSPI Flash (W25Q 系列)

- 适用于 STM32H7 等支持 QSPI 的型号
- 内存映射基址：`0x90000000`
- 支持页/扇区/块擦除，支持 DMA 读取
- 需在工程中定义 `hqspi` 句柄

## 其他模块

- **TMP117**：高精度 I2C 温度传感器
- **MX-22**：蓝牙 SPP/BLE 模块
- **RF24L01**：2.4GHz 无线收发
- **SoftSpi**：软件 SPI 实现
- **nanopb**：Protocol Buffers 编解码
- **MQTT**：MQTT 客户端（core_mqtt）
- **Tracealzer**：Percepio Tracealyzer 运行时追踪

## 接口说明

详见 [Inc/README.md](Inc/README.md) 中各模块的接口说明与适配方式。
