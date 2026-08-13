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
| `Inc/Bootloader/bootloader_flash.h` | 内部 Flash 擦写端口 |
| `Src/Bootloader/bootloader.c` | 读升级日志 → 装 Candidate / 回滚 → 跳 App |
| `Src/Bootloader/bootloader_flash_stm32f4.c` | F4 HAL 擦写实现 |
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
  → 校验 Candidate → 写入 App Flash → TRIAL_BOOT(count=1) → Jump

TRIAL_BOOT（未 Confirm）
  → count++；超限 → ROLLBACK_PENDING
  → 否则 Jump 当前 App

ROLLBACK_PENDING / ROLLING_BACK
  → 从 Rollback 槽刷回 → ROLLED_BACK → Jump

其它 + App 向量合法 → Jump
无有效 App → 返回错误（可死循环 / 闪灯）
```

## 烧录顺序

1. 先烧 Bootloader 到 `0x08000000`  
2. 再烧 Application 到 `0x08020000`（或合并 bin）  
3. OTA：仅写外部 Flash Candidate，复位后由 Bootloader 换槽
