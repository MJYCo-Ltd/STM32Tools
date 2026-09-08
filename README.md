# STM32Tools

STM32Tools 是一个基于 STM32CubeMX 的嵌入式模块工具集合，面向使用 STM32 微控制器开发的工程。仓库包含常用外设驱动（显示、传感器、摄像头、QSPI Flash、无线模组等）、AT 模组组包/解包工具、以及串口接收（DMA + IDLE）等实用中间件。

主要目标：
- 为不同显示（LCD / EPD）提供统一绘图与 SPI 接口
- 提供对常见传感器（AHT20 / TMP117 / ECSense）和无线模块（ML307 / EWM103 / RF24L01 / MX-22）的抽象接口
- EWM103 含 BluFi 指令（`BLEINIT` / `BLUFI`）；公共 AT 解析见 `AT/at_codec`
- Bootloader 逻辑库见 `Bootloader/`（外置 Flash OTA 安装 / IWDG 策略 / 试运行 / 跳转 App）
- 日历与北京时区（UTC+8）换算见 `Time/time_util`（`TimeUtil_UtcToBeijing` 等）
- 提供高可靠的 UART 接收（DMA ReceiveToIdle + 双缓冲）以降低 AT 回显/OK 分帧导致的丢字节

目录（精简）：

```
STM32Tools/
├── Inc/                    # 公共头文件、模块接口与移植说明（见 Inc/README.md）
├── Src/                    # 源文件实现
├── Bootloader/             # Bootloader 链接脚本、示例 main、说明
├── Test/                   # 测试（若有）
├── main.c                  # 旧版整板示例，仅在显式启用 legacy 宏时构建
├── LICENSE                 # 许可证
└── README.md               # 本文件
```

快速上手

1. 用 STM32CubeMX 生成工程框架或将本仓库源码整合到已有工程。
2. 在工程中配置 HAL/FreeRTOS（若使用），并根据硬件实现 Inc 中需要的用户配置（比如 `epd_user.c` / `lcd_st7789_user.c` / `ov5640_user.h` 中的引脚与 SPI/I2C 句柄）。
3. 在主循环或 FreeRTOS 任务中初始化所需模块。根目录 `main.c` 和
   `Src/freertos.c` 仅保留作旧硬件参考，不属于可复用库源码，也不应被
   应用工程自动收集。

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

## 协议解析与签名镜像校验

- `Inc/AT/ModuleFrameParser.h` / `Src/AT/ModuleFrameParser.c`：与具体模组无关的串口行解析、整数解析和长度帧解析，协议前缀、字段和帧尾由调用方传入。
- `Inc/Protocol/MqttLineCollector.h`：MQTT URC 分段行收集，回调和上下文由调用方提供；测试位于 `Test/mqtt_line_collector_test.c`。
- `Inc/Flash/SignedFirmware.h` / `Src/Flash/SignedFirmware.c`：固定 V1 格式镜像的 Ed25519 与 SHA-512 校验。通过 `SignedFirmwareReader` 注入读取和可选进度回调，通过 `SignedFirmwarePolicy` 注入公钥、产品/板型/硬件、魔数、目标地址和容量。通用库不引用 Agriculture、HAL、FreeRTOS 或具体 Flash 分区。
- `ThirdParty/monocypher/`：固定 Monocypher 4.0.2 的未修改源码、许可证和来源记录。构建验签模块时加入两个 `.c` 文件，并将该目录加入私有头文件路径。

镜像 V1 格式和业务协议未因迁移改变；验签不会授权安装、执行硬件动作或提供防回滚。应用负责安全窗口、存储锁、信任策略以及实际安装。Agriculture 中保留公钥、固件候选槽、看门狗适配、农业任务和执行结果存储格式。

2026-09-08 迁移后仅进行了文件摘要、依赖路径和差异静态检查，尚未重新编译；迁移前的测试结果不代表迁移后已验证。

## 2026-09-08 双 Bank 修复与测试归属

DualBankStore 初始化先选最新已提交记录，再校验长度和业务格式；旧 Bank 的不同格式不阻止新记录初始化，最新记录无效时也不回退为过期执行状态。通用事务测试已迁入 Test/dual_bank_transaction_test.c，测试 CMake 注册该目标，Agriculture 的测试工程直接引用同一文件。迁移时文件摘要一致，路径和差异静态检查通过；迁移后未重新编译或运行 C 测试。

农业旧检查点的 608/736 字节转换、板型与公钥策略、RTOS 堆大小及线程创建顺序属于具体应用，保留在 Agriculture/Hardware。使用该固件时必须一起更新两个仓库；当前 Agriculture 32 / 26.0.6 的重启修复仍待用户构建和上板验证。
