# Display（SPIDisplay / Graphics / LCD / EPD）

目标
- 提供可在 LCD 与 EPD 之间复用的 SPI/绘图接口，将硬件差异抽象到配置头文件（`epd_config.h`、`lcd_st7789_config.h`）。

主要文件
- `Inc/Display/SPIDisplay.h` — 统一 SPI 传输接口（内联实现，依赖用户宏）。
- `Inc/Display/Graphics.h`  — 基于 `DrawPixel` 的通用绘图接口（直线/圆/填充/字符）。
- `Inc/Display/EPD/` 和 `Inc/Display/LCD/` — 各屏驱动与板级配置头文件。

用户必须实现或配置的宏
- `SPI_SELECT()` / `SPI_UNSELECT()` — 片选操作（GPIO）
- `SPI_SEND_CMD()` / `SPI_SEND_DATA()` — DC 引脚控制（指令/数据）
- `DISPLAY_SPI_PORT` — HAL SPI 句柄（例如 `hspi1`）
- 可选：`USE_BUFFER` — 如果定义则 `SPIDisplay` 使用缓冲区 + DMA 传输大数据块

绘图 API 概览（由 Graphics 提供）
- `DrawPixel(x, y, color)`
- `DrawLine(x0,y0,x1,y1,color)` — Bresenham 算法
- `DrawRect` / `DrawFilledRect`
- `DrawCircle` / `DrawFilledCircle` — 中点圆算法
- `DrawTriangle` / `DrawFilledTriangle`
- `DrawChar(x,y,c,color)` / `DrawString(x,y,str,color)` — 基于 5x7 字体

EPD 使用要点
- 初始化：`EPD_Init(model, fastFresh)`；例如 `EPD_Init(EPD_THREE_COLOR, 1)`。
- 局部刷新：`EPD_DisplayPartial(x,y,w,h)` 用于降低刷新耗时与功耗。
- 绘图流程示例：

```c
EPD_Init(EPD_THREE_COLOR, 1);
EPD_PowerOn();
EPD_InitDrawBuffer(EPD_WHITE);
EPD_DrawRect(10, 10, 100, 60, EPD_BLACK);
EPD_ShowBuffer();
EPD_Update();
EPD_PowerOff();
```

LCD 使用要点（ST7789 示例）
- 在 `lcd_st7789_config.h` 中定义分辨率宏（`USING_135X240` / `USING_240X240` / `USING_170X320`）和 `DISPLAY_SPI_PORT`、DC、BL 引脚。
- 控制器驱动会包含对应配置头文件，并通过 `SPIDisplay.h` 调用统一的 `SPI_Send*` 接口。

性能与移植建议
- 如果目标平台 RAM 充足，开启 `USE_BUFFER` 并使用 DMA 分块传输大图块；否则在小分辨率或频繁更新场景下使用直接 SPI 传输。
- `DrawHLine` / `DrawVLine` 的第三个参数为终点坐标（不是长度）——移植或使用时注意。
- 若使用 EPD 的三色模式，请确认 `epd_uc8253.c` 中的颜色常量和 `epd_graphics.h` 保持一致。

调试提示
- 局部刷新坐标、缓冲区大小和 DMA 分块大小是常见的移植问题来源；遇到显示错位或卡顿先检查这些参数。
- 在 FreeRTOS 下，将显示刷新放到低优先级任务并使用消息队列触发刷新以避免阻塞高优先级任务。

更多示例和配置请参考仓库中的 `Inc/Display` 与 `Src/Display` 子目录。
