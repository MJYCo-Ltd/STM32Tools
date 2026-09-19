# Legacy STM32F103 example

本目录保存 STM32Tools 早期的 STM32F103 + FreeRTOS 整板示例：

- `main.c`
- `freertos.c`
- `stm32f103_c8t6.resc`

这些文件依赖对应 CubeMX 工程生成的 `main.h`、HAL 句柄和板级引脚，不属于可复用库源码。应用工程不应递归收集本目录源码。

示例中的 `freertos.c` 仍要求显式定义 `STM32TOOLS_BUILD_LEGACY_EXAMPLE`，用于防止误加入产品固件。
