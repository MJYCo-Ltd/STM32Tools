# Camera — OV5640 / OV2640 使用与移植说明

主要文件
- `Inc/Camera/ov5640_user.h` — 用户层封装，需实现硬件相关回调与引脚定义。
- `Src/Camera/ov5640.c`、`Src/Camera/ov2640.c` — 驱动实现（DCMI / I2C 配置）。
- `Src/Camera/OV5640_INIT_Config_vs_Common.md` — 两套 OV5640 配置对比（240×240 参考配置）。

初始化顺序（OV5640）
```
OV5640_USER_HwReset() -> OV5640_USER_RegisterBusIO() -> OV5640_USER_SoftReset() ->
OV5640_CAMERA_Driver.ReadID() -> OV5640_CAMERA_Driver.Init()
```

用户需提供
- CAMERA_PWDN_GPIO_Port/Pin、CAMERA_RESET_GPIO_Port/Pin（在 main.h 中定义）
- `OV5640_USER_RegisterBusIO()` 中注册 I2C/SCCB 的读写回调
- 如使用 DCMI，确保对应 DMA/帧缓冲（SDRAM 或 SRAM）正确配置

分辨率与像素格式
- 支持：160x120、320x240、480x272、640x480、800x480、400x300
- 像素格式：RGB565、RGB888、YUV422、Y8、JPEG

移植建议
- 先在板子上读 ID（ReadID）验证总线与复位电路正确；
- 若使用 JPEG，需额外确认 DMA 与存储缓冲区大小；
- 摄像头与显示联动时注意像素格式转换与内存占用。

OV2640 要点
- OV2640 驱动接口与 OV5640 类似，但分辨率/寄存器集不同；
- 需提供 BSP 层的 camera.h 与相应的 HW 抽象。

调试提示
- 使用示波器或逻辑分析仪确认 SCCB/I2C 时序与 DCMI 像素时钟（PCLK）；
- 若出现图像偏移或花屏，先检查 DMA 的数据宽度（16/8/32-bit）与帧缓冲地址对齐。
