# STM32Tools

STM32Tools 是一个基于 STM32CubeMX 的嵌入式模块工具集合，面向使用 STM32 微控制器开发的工程。仓库包含常用外设驱动（显示、传感器、摄像头、QSPI Flash、无线模组等）、AT 模组组包/解包工具、以及串口接收（DMA + IDLE）等实用中间件。

主要目标：
- 为不同显示（LCD / EPD）提供统一绘图与 SPI 接口
- 提供对常见传感器（AHT20 / TMP117 / ECSense）和无线模块（ML307 / EWM103 / RF24L01 / MX-22）的抽象接口
- 提供高可靠的 UART 接收（DMA ReceiveToIdle + 双缓冲）以降低 AT 回显/OK 分帧导致的丢字节

目录（精简）：

```
STM32Tools/
├── Inc/                    # 公共头文件、模块接口与移植说明（见 Inc/README.md）
├── Src/                    # 源文件实现
├── Test/                   # 测试（若有）
├── main.c                  # 示例入口，展示各模块初始化/使用方式
├── LICENSE                 # 许可证
└── README.md               # 本文件
```

快速上手

1. 用 STM32CubeMX 生成工程框架或将本仓库源码整合到已有工程。
2. 在工程中配置 HAL/FreeRTOS（若使用），并根据硬件实现 Inc 中需要的用户配置（比如 `epd_user.c` / `lcd_st7789_user.c` / `ov5640_user.h` 中的引脚与 SPI/I2C 句柄）。
3. 在主循环或 FreeRTOS 任务中调用示例 `main.c` 中展示的初始化流程。

示例（EPD 全屏刷写）：

```c
EPD_Init(EPD_THREE_COLOR, 1);
EPD_PowerOn();
EPD_Clear(EPD_WHITE);
EPD_Update();
EPD_PowerOff();
```

重要注意事项

- FreeRTOS：不要在任务栈中定义大数组；动态分配请使用 `pvPortMalloc` 并在任务上下文中申请。
- UART：使用仓库提供的 `UartReceive` 模块（DMA ReceiveToIdle + 双缓冲），在 IDLE 事件中先重启 DMA 再入队以降低丢字节风险。
- QSPI Flash：使用内存映射模式需定义 `hqspi` 并保证目标芯片支持。
- 摄像头：OV5640/OV2640 需提供摄像头相关引脚与 BSP 适配函数（见 `Inc/Camera/ov5640_user.h`）。

进一步阅读

- 模块接口/移植说明请打开：Inc/README.md
- 具体模块实现位于 Src/ 下的对应子目录

贡献

欢迎提交 Issue 或 Pull Request。若在移植到你目标板时遇到问题，请在 Issue 中说明 MCU 型号、编译器/IDE 与最小复现步骤。
