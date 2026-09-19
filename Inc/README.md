# STM32Tools 接口与迁移索引

接口签名以本版本头文件为准。本页只维护入口和跨模块约定，不再复制另一套会过期的函数表。

## 已整理模块

| 模块 | 接口／详细说明 | 产品必须提供 |
|---|---|---|
| 按键 | [Button.h](Button.h)、[可编译用法](../Examples/ApiUsage/) | 双边沿 EXTI、对象生命周期、任务唤醒与期限调度 |
| AHT20 / TMP117 | [传感器接入](SENSOR_DRIVERS.md) | 每设备 I2C_Bus；AHT20 还需毫秒延时回调 |
| ST7305 显示 | [lcd_st7305.h](Display/LCD/lcd_st7305.h)、[显示说明](Display/README.md) | 总线、面板 Profile、Reset、延时、缓冲区、旋转方向 |
| AT 恢复 | [AtChannelRecovery.h](AT/AtChannelRecovery.h) | 模组命令、Ready 判定、硬件动作与业务策略 |
| AT 组帧／编解码 | [AT](AT/) | 接收字节、长度、会话对象与超时策略 |
| 存储／验签 | [Flash](Flash/) | 后端、事务、分区、产品信任策略 |
| 总线／协议 | [Bus](Bus/)、[Protocol](Protocol/) | 具体外设和执行策略 |
| 通用工具／时间 | [Common.h](Common.h)、[Time](Time/) | 调用参数及产品时区规则 |

## 必须同步迁移的调用

**按键：** GPIO 必须先配置为 `GPIO_MODE_IT_RISING_FALLING`。按下和松开均调用
`Button_NotifyExtiPin()` 并通知拥有者任务。任务调用 `Button_Process()`，然后用
`Button_NextWakeDelay()` 决定下一次必要处理时间。返回值单位是毫秒；`0` 为立即处理，
`BUTTON_WAIT_FOREVER` 表示仅等待下一次边沿。转换到 RTOS tick 时要保留这两种特殊值。
不能仅开启下降沿并永久等待。注册后检查 `Button_IsRegistered()`，非静态对象离开作用域前调用
`Button_Deinit()`。业务回调不在 ISR 中运行。Button 当前仍使用 STM32/CMSIS 类型，不是纯平台无关模块。

**传感器：** 旧的全局总线接口已经移除。用 `AHT20_DeviceInit()` 绑定资源，再调用
`AHT20_Initialize(&device)` 和 `AHT20_Read(&device, &data)`。TMP117 使用
`TMP117_DeviceInit()`、`TMP117_GetTemperature(&device, &temperature)`；返回原始值和摄氏度，
不判断体温范围。一个物理 AHT20 的地址为 `0x38`，多实例不等于能在同一总线上使用相同地址。

**显示：** 产品先绑定 `ST7305_Binding`，检查 `LCD_ST7305_Initialize()` 返回值。
新代码使用返回状态的 `LCD_ST7305_Reset/Refresh/RefreshArea`，通过
`LCD_ST7305_IsReady()` 查询就绪。复位和传输失败使就绪失效，恢复由拥有者串行执行。
旧 `void LCD_*` 仅为兼容，不应用于需要错误反馈的恢复流程。延时由产品注入；只有当产品选择
RTOS 延时时，才要求在对应调度环境中初始化，通用驱动本身不要求 FreeRTOS。

## 尚未完成统一化的模块

`Auxiliary`、`UartReceive`、部分 Bootloader 平台代码，以及 ST7789/EPD 旧后端仍有
HAL、全局句柄或宏配置依赖。保留这些模块不表示已经满足显式依赖注入约定。
移植前阅读各自头文件／源码，不要套用 ST7305 的绑定方式或自动搬入板级参数。
Bootloader 的现有平台假设见 [Bootloader 说明](../Bootloader/README.md)。

## 验证入口

[主机测试说明](../Test/README.md)；[编译检查的 API 示例](../Examples/ApiUsage/)。
新增测试可执行文件必须注册同名 CTest；配置时会检查漏项。主机测试、ARM 构建和上板测试是三个不同的验证层次。
