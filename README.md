# STM32Tools

STM32Tools 是面向 STM32 项目的可复用嵌入式组件库。仓库提供协议解析、存储算法、传感器/通信模组驱动、显示驱动、按键状态机、Bootloader 核心逻辑和产品工程边界说明。

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
├── Test/                   宿主测试
├── ThirdParty/              固定版本第三方依赖
├── Examples/BootloaderSTM32F411/  Bootloader 参考工程
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
- `Bootloader`：安装、试运行、回滚和看门狗策略组件；
- `Time/`、`Common`、`ValueFormat`：通用工具。

## 快速接入

1. 用 STM32CubeMX 生成产品工程；
2. 仅选择产品实际使用的 STM32Tools 源文件或 CMake Target；
3. 在产品 Board/BSP 层绑定 SPI、I2C、UART、GPIO、RTC、Flash 分区和看门狗；
4. 在产品任务中调用通用模块，不要把业务规则写回驱动层；
5. 运行 `Test/` 中对应的宿主测试，再进行交叉编译和上板验证。

## Bootloader 说明

可复用的 Bootloader 逻辑库，供产品工程链接；它不包含完整 CubeMX 工程，产品侧应建裸机工程并链入本库源码。

### 示例 Flash 布局

以下仅为示例，真实地址必须由产品工程通过 `STM32TOOLS_BOOT_CONFIG_HEADER` 或显式宏定义 `BOOTLOADER_FLASH_BASE`、`FLASH_SIZE`、`BOOTLOADER_APP_FLASH_BASE`、`APP_FLASH_SIZE`、`BOOTLOADER_SRAM_BASE`、`SRAM_SIZE` 来指定，不允许隐式使用另一块板的默认地址：

| 区 | 地址 | 大小 | 扇区 |
|---|---|---|---|
| Bootloader | `0x08000000` | 128 KB | 0–4 |
| Application | `0x08020000` | 384 KB | 5–7 |

### 目录与作用

| 路径 | 作用 |
|---|---|
| `Inc/Bootloader/bootloader.h` | `Bootloader_Run` / `JumpToApp` / `InstallSlot` |
| `Inc/Bootloader/bootloader_flash.h` | 内部 Flash 擦写端口（按扇区擦除 + 喂狗钩子） |
| `Inc/Bootloader/bootloader_iwdg.h` | IWDG 启动 / 喂狗 / 复位原因 / SafeHold |
| `Inc/Bootloader/bootloader_policy.h` | 可宿主测试的安装 / 试运行 / 风暴回退策略 |
| `Src/Bootloader/bootloader.c` | 读升级日志 → 持久化 → 装 Candidate / 回滚 → 跳 App |
| `Src/Bootloader/bootloader_flash_stm32f4.c` | F4 HAL 擦写实现 |
| `Src/Bootloader/bootloader_iwdg.c` | STM32 IWDG（prescaler 128 / reload 4095 ≈ 16 s） |
| `Src/Bootloader/bootloader_policy.c` | 复位原因与阶段尝试次数决策 |
| `Examples/BootloaderSTM32F411/STM32F411xx_BOOT.ld` | Bootloader 链接脚本 |
| `Examples/BootloaderSTM32F411/example_main.c` | 产品侧 `main` 骨架（`#if 0`） |

依赖：`Flash/storage_*`、`W25Q`、`Common`、STM32 HAL Flash。

### 产品侧接入要求

1. App 链接脚本 `STM32F411xx_FLASH.ld`：`FLASH ORIGIN=0x08020000, LENGTH=384K`；
2. `system_stm32f4xx.c`：开启 `USER_VECT_TAB_ADDRESS`，`VECT_TAB_OFFSET=0x20000`；
3. Bootloader 目标必须单独使用 `STM32F411xx_BOOT.ld`（`ORIGIN=0x08000000`）；
4. 下载镜像到 W25Q Candidate，写 Manifest（`target_address=0x08020000`），状态置 `INSTALLING` 后复位；
5. App 可用后调用 `UpgradeControl_OnHealthyBoot()`，不要在 `main()` 一开始就确认升级。

### IWDG 与策略常量

| 宏 | 默认 | 含义 |
|---|---|---|
| IWDG prescaler / reload | 128 / 4095 | 典型超时约 16.4 s，只覆盖单个 128 KB 扇区擦除 |
| `BOOTLOADER_MAX_TRIAL_BOOTS` | 3 | 未健康确认的试运行次数 |
| `BOOTLOADER_MAX_PHASE_ATTEMPTS` | 3 | 同一安装/回滚阶段允许擦写次数 |
| `BOOTLOADER_MAX_WATCHDOG_STORM` | 8 | 已确认镜像连续 IWDG 次数，超限回滚或 SafeHold |

`UpgradeStatePayload` 还含 `reset_reason`、`watchdog_resets`、`phase_attempts`。`persist` 为真时必须先写入外部 Flash，再擦内部 Flash 或跳 App。

### 状态机简述

```text
INSTALLING / CANDIDATE_VALID
  → 先写入阶段计数
  → 按扇区擦除 App Flash，再编程
  → TRIAL_BOOT(count=1) → Jump（App 接管 IWDG）
  → 擦写被 IWDG 打断：下次仍 INSTALLING；attempts 用尽 → ROLLBACK

TRIAL_BOOT（App 未 Confirm / OnHealthyBoot）
  → count++；超限 → ROLLBACK_PENDING
  → 否则 Jump 当前 App

ROLLBACK_PENDING / ROLLING_BACK
  → 先持久化状态，再回滚
  → 成功 ROLLED_BACK → Jump
  → attempts 用尽或无回滚包：FAILED

CONFIRMED / IDLE + 连续 IWDG 达到 max_watchdog_storm
  → ROLLBACK；无回滚包则 SafeHold
```

### 烧录顺序

1. 先烧 Bootloader 到 `0x08000000`；
2. 再烧 Application 到 `0x08020000`（或合并 bin）；
3. OTA：仅写外部 Flash Candidate，复位后由 Bootloader 换槽。

宿主测试：`Test/storage_test` 中包含 `TestBootloaderPolicy`，覆盖安装 attempts、IWDG+PIN 不误清、风暴回滚、POR 清 `watchdog_resets` 等场景。

## 典型边界示例

- `Button.c` 负责按键消抖和事件分类；具体按键引脚与菜单动作属于产品工程。
- `W25Q` 负责 SPI NOR 指令；具体 SPI/CS、电压门限、互斥锁和分区属于产品工程。
- `SignedFirmware` 负责验签算法；公钥、产品身份、候选槽和安装授权属于产品工程。
- `AtChannelRecovery` 负责恢复状态推进；具体重启命令、Ready URC、硬件 Reset 和退避策略由模组/产品适配。

## 旧整板示例

原根目录 `main.c`、`Src/freertos.c` 和 `stm32f103_c8t6.resc` 已移到 `Examples/LegacyF103/`。它们只用于历史硬件参考，不应被应用工程自动收集，也不代表公共库支持该整板工程。

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
