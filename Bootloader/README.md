# Bootloader（STM32Tools）

可复用的 Bootloader 逻辑库，供 Agriculture 等产品工程链接。  
**不**包含完整 CubeMX 工程：产品侧建裸机工程，链入本目录源码即可。

## 默认内部 Flash 布局（STM32F411 512KB）

| 区 | 地址 | 大小 | 扇区 |
|---|---|---|---|
| Bootloader | `0x08000000` | 128 KB | 0–4 |
| Application | `0x08020000` | 384 KB | 5–7 |

宏见 `Inc/Bootloader/bootloader_memmap.h`（与 Agriculture `StorageLayout.h` 保持一致）。

## 源文件

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
| `Bootloader/STM32F411xx_BOOT.ld` | Bootloader 链接脚本 |
| `Bootloader/example_main.c` | 产品侧 `main` 骨架（`#if 0`） |

依赖：`Flash/storage_*`、`W25Q`、`Common`、STM32 HAL Flash。

产品侧：Agriculture [`Hardware/Bootloader`](../../Agriculture/Hardware/Bootloader/README.md) 已提供可编译裸机工程（链本库）。

1. `STM32F411xx_FLASH.ld`：`FLASH ORIGIN=0x08020000, LENGTH=384K`  
2. `system_stm32f4xx.c`：开启 `USER_VECT_TAB_ADDRESS`，`VECT_TAB_OFFSET=0x20000`  
3. 下载镜像到 W25Q Candidate，写 Manifest（`target_address=0x08020000`），状态置 `INSTALLING` 后复位  
4. 试运行成功后调用 `UpgradeControl_Confirm()`

## 状态机（与 `storage_upgrade.h` 对齐）

```text
INSTALLING / CANDIDATE_VALID
  → 先把 phase_attempts++ 写入升级日志
  → 按扇区擦除 App Flash（扇区间喂狗）→ 编程
  → TRIAL_BOOT(count=1) → Jump（IWDG 继续跑，由 App 接管）
  → 擦写被 IWDG 打断：下次仍 INSTALLING；attempts 用尽 → ROLLBACK

TRIAL_BOOT（App 未 Confirm / OnHealthyBoot）
  → count++；超限 → ROLLBACK_PENDING
  → 否则 Jump 当前 App

ROLLBACK_PENDING / ROLLING_BACK
  → 同样先持久化 attempts，再按扇区刷 Rollback
  → 成功 ROLLED_BACK → Jump
  → attempts 用尽或无 Rollback 槽：FAILED；
    若是 IWDG 风暴则 SafeHold，不再跳已确认但反复复位的 App

CONFIRMED / IDLE + 连续 IWDG 达到 max_watchdog_storm
  → ROLLBACK；无回滚包则 SafeHold

其它 + App 向量合法 → Jump
无有效 App → 返回错误；产品侧 SafeHold（喂狗循环，禁止空转等复位）
```

### 为什么不能只在 `main()` 开头启动 IWDG 然后循环喂狗

- IWDG 一旦启动无法关闭，跳 App 后必须由 App **接管**（尽早 Feed，调度后由控制循环 Kick，健康后 `OnHealthyBoot`）。
- 内部 Flash 128 KB 扇区擦除最差约 4 s，整片 App 区一次擦除会超过超时 → 半擦除 + 反复安装 = **复位风暴**。必须按扇区擦并在扇区间喂狗。
- 擦除/跳转前必须把状态写进外部 Flash；IWDG 复位后才能区分“安装中 / 试运行 / 已确认崩溃”。
- 确认后的 App 若卡死，IWDG 会一直复位同一镜像。用 `watchdog_resets` 计数，超限回滚或 SafeHold。

App 侧：不要单独做高优先级“只喂狗”任务，那会掩盖业务挂死。Agriculture 在 `WatchdogService` 里于启动路径 Feed，在 `ButtonBusiness_Run` 控制循环 Kick。

## 烧录顺序

1. 先烧 Bootloader 到 `0x08000000`  
2. 再烧 Application 到 `0x08020000`（或合并 bin）  
3. OTA：仅写外部 Flash Candidate，复位后由 Bootloader 换槽
