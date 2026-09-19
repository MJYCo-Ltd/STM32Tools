# STM32Tools

STM32Tools 是面向 STM32 项目的可复用嵌入式组件库。仓库提供协议解析、存储算法、传感器/通信模组驱动、显示驱动、按键状态机、Bootloader 核心逻辑以及 STM32 HAL/CMSIS-RTOS 适配代码。

## 代码边界

放入 STM32Tools 的代码应满足以下原则：

- 不依赖具体产品名称、服务器 Topic、业务流程或 Flash 分区地址；
- 不硬编码具体板卡引脚、UART/SPI/I2C 实例；
- 板级资源通过参数、上下文、回调或 Port 层注入；
- 核心状态机和协议逻辑可在宿主机上独立测试。

具体产品的引脚、时钟、分区、硬件能力、看门狗策略和业务流程应保留在产品仓库，例如 `Agriculture_Hardware`。

## 目录

```text
STM32Tools/
├── Inc/                    公共头文件和模块接口
├── Src/                    可复用实现及平台 Port
├── Bootloader/             Bootloader 说明、链接脚本和接入示例
├── Test/                   宿主测试
├── ThirdParty/              固定版本第三方依赖
├── Examples/LegacyF103/    旧 STM32F103 整板示例，不属于库源码
├── LICENSE
└── README.md
```

## 主要模块

- `AT/`：AT 文本编解码、分行/长度帧解析、通用 AT 通道恢复状态机；
- `ML307/`、`EWM103/`：通信模组命令组包和响应解析，不直接决定产品业务；
- `Flash/`：NOR Backend、分区、掉电安全记录、DualBank、固件槽、签名验证；
- `Protocol/`：Modbus、HTTP Range、MQTT 行收集等协议组件；
- `Bus/`：I2C、RS485、Modbus 队列等总线抽象和 STM32 Port；
- `Display/`：显示控制器、统一 SPI 显示和绘图接口；
- `Button`：双边沿 EXTI、软件消抖及短按/长按/超长按状态机；
- `Bootloader/`：安装、试运行、回滚和看门狗策略组件；
- `Time/`、`Common`、`ValueFormat`：通用工具。

## 快速接入

1. 用 STM32CubeMX 生成产品工程；
2. 仅选择产品实际使用的 STM32Tools 源文件或 CMake Target；
3. 在产品 Board/BSP 层绑定 SPI、I2C、UART、GPIO、RTC、Flash 分区和看门狗；
4. 在产品任务中调用通用模块，不要把业务规则写回驱动层；
5. 运行 `Test/` 中对应的宿主测试，再进行交叉编译和上板验证。

## 典型边界示例

- `Button.c` 负责按键消抖和事件分类；具体按键引脚与菜单动作属于产品工程。
- `W25Q` 负责 SPI NOR 指令；具体 SPI/CS、电压门限、互斥锁和分区属于产品工程。
- `SignedFirmware` 负责验签算法；公钥、产品身份、候选槽和安装授权属于产品工程。
- `AtChannelRecovery` 负责恢复状态推进；具体重启命令、Ready URC、硬件 Reset 和退避策略由模组/产品适配。

## 旧整板示例

原根目录 `main.c`、`Src/freertos.c` 和 `stm32f103_c8t6.resc` 已移到 `Examples/LegacyF103/`。它们只用于历史硬件参考，不应被应用工程自动收集，也不代表公共库接口。

## 测试

```bash
cmake -S Test -B build/test
cmake --build build/test
ctest --test-dir build/test --output-on-failure
```

关键测试包括按键状态机、AT 通道恢复、MQTT 行收集、DualBank 事务和存储/Bootloader 策略。

## 许可证

见 `LICENSE`。

## Consolidated correctness and component contracts

See [ENGINEERING_CONTRACTS](Inc/ENGINEERING_CONTRACTS.md) for checked Auxiliary,
UART loss/fairness, bounded button history, text MQTT boundaries, HealthMonitor
and named CMake targets. These changes are source/ABI migrations, not an
automatic hardware or low-power port for every STM32 family.
