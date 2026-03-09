# STM32Tools 接口说明

本文档说明 `Inc` 目录下各头文件提供的接口及移植适配方式。

---

## 基础类型 (Display/Display.h)

| 类型 | 说明 |
|------|------|
| `COLOR` | RGB 颜色 `{uRed, uGreen, uBlue}` |
| `Pixel` | 像素 `{x, y, color}` |
| `ROTATION` | 旋转方向 `NO_ROTATION/90/180/270` |

**工具函数**：`COLOR_EQUAL()`, `ColorToRGB565()`

---

## SPI 显示统一接口 (Display/SPIDisplay.h)

EPD 与 LCD 共用的 SPI 传输接口，头文件内联实现。**包含顺序**：先包含用户配置（`epd_user.c` 或 `lcd_user.c`），再包含 `SPIDisplay.h`。

| 接口 | 说明 |
|------|------|
| `SPI_SendCommand(cmd)` | 写指令（DC 低） |
| `SPI_SendData(data)` | 写单字节数据（DC 高） |
| `SPI_SendBuffer(buff, len)` | 写数据缓冲区（DC 高，支持 64K 分块及 DMA） |

**用户配置需定义**：

| 宏 | 说明 |
|----|------|
| `SPI_SELECT` | 片选有效 |
| `SPI_UNSELECT` | 片选释放 |
| `SPI_SEND_CMD` | DC 置为指令模式 |
| `SPI_SEND_DATA` | DC 置为数据模式 |
| `DISPLAY_SPI_PORT` | SPI 句柄 |
| `USE_BUFFER` | 可选，大块数据使用 DMA |

---

## 显示 - LCD (Display/LCD/)

### lcd.h

| 接口 | 说明 |
|------|------|
| `LCD_Init()` | 初始化 LCD（需 FreeRTOS 启动后调用） |
| `LCD_Reset()` | 复位 |
| `LCD_SetRotation(m)` | 设置旋转方向 |
| `LCD_SetAddressWindow(x0,y0,x1,y1)` | 设置显存窗口 |
| `LCD_Refresh()` | 刷新帧缓冲区到屏幕 |
| `LCD_InvertColors(invert)` | 全屏颜色反显开关 |
| `LCD_TearEffect(tear)` | 撕裂效应线开关 |

### 用户配置 (Display/LCD/lcd_user.c)

需在工程中提供或修改：

| 配置项 | 说明 |
|--------|------|
| `DISPLAY_SPI_PORT` | SPI 句柄，如 `hspi4` |
| `ST7789_DC_PORT/PIN` | DC 引脚（数据/指令选择） |
| `LCD_BlackLight_GPIO_Port/Pin` | 背光引脚 |
| `USING_135X240` / `USING_240X240` / `USING_170X320` | 分辨率三选一 |
| `CFG_NO_CS` | 无硬件 CS 时定义 |
| `CFG_NO_REST` | 无硬件 RST 时定义 |
| `USE_BUFFER` | 使用 DMA 传输时定义（需足够 RAM） |

---

## 显示 - EPD 墨水屏 (Display/EPD/)

### epd.h

| 接口 | 说明 |
|------|------|
| `EPD_Init(model, fastFresh)` | 初始化，model: `EPD_TWO_COLOR`/`EPD_THREE_COLOR` |
| `EPD_PowerOn()` / `EPD_PowerOff()` | 开关机 |
| `EPD_Rest()` | 复位（DeepSleep 后需调用） |
| `EPD_DeepSleep()` | 进入深度休眠 |
| `EPD_Clear(color)` | 清屏 |
| `EPD_Update()` | 刷新到屏幕 |
| `EPD_InitDrawBuffer(color)` | 初始化绘图缓冲区 |
| `EPD_ShowBuffer()` | 将缓冲区内容显示 |
| `EPD_DisplayPartial(x,y,w,h)` | 局部刷新 |
| `EPD_IsOk()` | 自检 |
| `EPD_GetInnerTemp()` | 获取内部温度 |

### 用户配置 (Display/EPD/epd_user.c)

需在工程中提供：

| 配置项 | 说明 |
|--------|------|
| `EPD_WIDTH` / `EPD_HEIGHT` | 屏幕尺寸 |
| `EPD_BUFFER_SIZE` | 缓冲区大小 |
| `CS_GPIO_Port` / `CS_Pin` | 片选引脚 |
| `DC_GPIO_Port` / `DC_Pin` | 数据/指令选择 |
| `RST_GPIO_Port` / `RST_Pin` | 复位引脚 |
| `BUSY_GPIO_Port` / `BUSY_Pin` | 忙状态引脚 |
| `DISPLAY_SPI_PORT` | SPI 句柄，如 `hspi1` |

epd_user.c 需定义 `SPI_SELECT`、`SPI_UNSELECT`、`SPI_SEND_CMD`、`SPI_SEND_DATA` 供 SPIDisplay.h 使用。

**可重写弱函数**：

| 函数 | 默认行为 |
|------|----------|
| `EPD_Rest()` | 拉低 RST 10ms → 拉高 10ms |
| `EPD_WaitUntilIdle()` | 轮询 BUSY 直到高电平 |

---

## 绘图 (Display/Graphics.h)

基于 `DrawPixel` 的通用绘图接口，LCD 与 EPD 共用同一套接口，底层由各自 `DrawPixel` 实现区分。

| 接口 | 说明 |
|------|------|
| `DrawPixel(pPixel)` | 画点 |
| `DrawLine(x0,y0,x1,y1,color)` | 直线（Bresenham） |
| `DrawHLine(x0,y0,x1,color)` | 水平线，x1 为终点坐标 |
| `DrawVLine(x0,y0,y1,color)` | 垂直线，y1 为终点坐标 |
| `DrawRect` / `DrawFilledRect` | 矩形 / 填充矩形 |
| `DrawCircle` / `DrawFilledCircle` | 圆 / 填充圆（中点算法） |
| `DrawTriangle` / `DrawFilledTriangle` | 三角形 / 填充三角形 |
| `DrawChar(x,y,c,color)` | 单字符（5x7 字体） |
| `DrawString(x,y,str,color)` | 字符串 |

---

## 通用工具 (Common.h)

| 接口 | 说明 |
|------|------|
| `CalCRC16(buffer, len)` | 计算 CRC16 |
| `AddCRC16(buffer, len, isLittleEndian)` | 在缓冲区末尾添加 CRC |
| `JudgeCRC16(buffer, len, isLittleEndian)` | 校验 CRC |
| `ConvertBigEndian2Double/Word/HalfWord` | 大端转本地 |
| `ConvertDouble/Word/HalfWord2BigEndian` | 本地转大端 |
| `ConvertLittleEndian2*` / `Convert*2LittleEndian` | 小端转换 |
| `Rand_range(start, end, align)` | 随机范围 [start, end) |
| `Swap(a, b)` | 交换两个 uint16_t |

**类型**：`DOUBLE_DATA`, `WORD_DATA`, `HALF_WORD_DATA`

---

## 平台基础 (Base.h)

| 宏 | 说明 |
|----|------|
| `YTY_DELAY_MS(ms)` | 延时（FreeRTOS 下为 `osDelay`） |
| `YTY_MALLOC` / `YTY_FREE` | 内存分配（FreeRTOS 下为 `pvPortMalloc`/`vPortFree`） |
| `PLATFORM_STM32` | STM32 系列时定义 |

---

## 辅助功能 (Auxiliary.h)

| 接口 | 说明 |
|------|------|
| `SendDebugInfo(pData, len)` | 发送调试信息 |
| `RequestSpace(size)` | 申请空间 |
| `RecycleSpace(ptr)` | 释放空间 |
| `ReadFlash()` | 读取 Flash |
| `Enter_Sleep()` / `Enter_Stop()` | 进入休眠/停止模式 |
| `EnterLowPowerMode(mode, counter, clock)` | 低功耗模式 |
| `GetStatus()` | 获取状态（RAM、CPU 等） |

---

## 串口接收 (UartReceive.h)

| 接口 | 说明 |
|------|------|
| `InitUartCount(max)` | 初始化串口数量 |
| `AddUart(huart, callback)` | 添加串口及回调 |
| `GetUart(id)` | 获取串口句柄 |
| `GetUartIOInfo(id)` | 获取收发统计 |
| `BeginReceiveUartInfo(id)` / `StopReceiveUartInfo(id)` | 启停接收 |
| `ProcessUart()` | 定时处理（需在任务中调用） |
| `GetUartCount()` | 获取串口数量 |

---

## 软件 SPI (SoftSpi.h)

| 接口 | 说明 |
|------|------|
| `SPI_Write(data)` | 发送单字节 |
| `SPI_WriteBuffer(buf, len)` | 发送缓冲区 |

需用户实现或按硬件 SPI 封装。

---

## TMP117 温度传感器 (TMP/tmp117.h)

| 接口 | 说明 |
|------|------|
| `TMP117_GetTemperature(addr7, temp)` | 读取温度 |
| `TMP117_SetWorkMode(addr7, mode)` | 设置工作模式 |

**地址**：`TMP117_ADDR_GND`(0x48) 等  
**模式**：`TMP117_MODE_CONTINUOUS` / `SHUTDOWN` / `ONE_SHOT`

---

## MX-22 蓝牙模块 (MX/mx22.h)

| 接口 | 说明 |
|------|------|
| `MX22_Init()` | 初始化 |
| `MX22_EnterCommandMode()` / `MX22_EnterDataMode()` | 模式切换 |
| `MX22_GetVersion()` | 获取版本 |
| `MX22_GetMAC()` | 获取 MAC |
| `MX22_SetSPPName()` / `MX22_SetBLEName()` | 设置名称 |
| `MX22_SetBaudrate()` | 设置波特率 |
| `MX22_SetRadioMode()` | 设置无线模式 |
| `MX22_EnableSPP()` / `MX22_EnableBLE()` | 启用 SPP/BLE |
| `MX22_Disconnect()` | 断开连接 |
| `MX22_IsConnected()` | 是否已连接 |
| `MX22_SendData()` | 发送数据 |
| `MX22_EnablePairing()` / `MX22_SetPairingPin()` | 配对相关 |
| `MX22_WaitForConnection(timeout_ms)` | 等待连接 |

---

## 爱氪森传感器 (ECSense.h)

| 接口 | 说明 |
|------|------|
| `ModifyAddr(newAddr, buf)` | 构建修改 Modbus 地址命令 |
| `ModifyAddrResponse()` | 解析修改地址响应 |
| `ReadDS4Value(addr, buf)` | 构建读取数据命令 |
| `DS4Sleep()` / `DS4Wakeup()` | 睡眠/唤醒命令 |
| `ReadDS4ValueResponse()` | 解析传感器数据 |
| `GetShowInfo()` | 将数据转为可读字符串 |

---

## EEPROM (L051C8T6/Eeprom.h)

仅适用于 STM32L051 内置 EEPROM。

| 接口 | 说明 |
|------|------|
| `EEPROM_WriteBytes/HalfWords/Words()` | 写入 |
| `EEPROM_ReadBytes/HalfWords/Words()` | 读取 |
| `EEPROM_Erase()` | 擦除 |

---

## OV5640 摄像头 (Camera/ov5640.h)

500 万像素摄像头，I2C/SCCB 配置寄存器，DCMI 接收图像。

| 接口 | 说明 |
|------|------|
| `OV5640_RegisterBusIO(pObj, pIO)` | 注册 I2C 读写回调 |
| `OV5640_Init(pObj, Resolution, PixelFormat)` | 初始化 |
| `OV5640_ReadID(pObj, Id)` | 读取器件 ID |
| `OV5640_SetResolution` / `OV5640_SetPixelFormat` | 设置分辨率、像素格式 |
| `OV5640_SetLightMode` / `OV5640_SetColorEffect` | 光照模式、色彩特效 |
| `OV5640_SetBrightness` / `SetSaturation` / `SetContrast` | 亮度、饱和度、对比度 |
| `OV5640_MirrorFlipConfig` / `OV5640_ZoomConfig` | 镜像翻转、变焦 |
| `OV5640_Start` / `OV5640_Stop` | 启动/停止采集 |

**分辨率**：`OV5640_R160x120`、`OV5640_R320x240`、`OV5640_R480x272`、`OV5640_R640x480`、`OV5640_R800x480`  
**像素格式**：`OV5640_RGB565`、`OV5640_RGB888`、`OV5640_YUV422`、`OV5640_Y8`、`OV5640_JPEG`

需实现 `OV5640_IO_t`（Init、WriteReg、ReadReg 等），详见 `ov5640_user.h`。

---

## OV2640 摄像头 (Camera/ov2640.h)

200 万像素摄像头，I2C 配置，DCMI 接收。需提供 `camera.h` 等 BSP 依赖。

---

## QSPI Flash (QSPIFlash.h)

W25Q 系列 QSPI Flash，适用于 STM32H7 等支持 QSPI 的型号。

| 接口 | 说明 |
|------|------|
| `QSPI_Flash_Init()` | 初始化 |
| `QSPI_Flash_ReadID()` | 读取器件 ID |
| `QSPI_Flash_Read(addr, buf, len)` | 读取数据 |
| `QSPI_Flash_Read_DMA(addr, buf, len)` | DMA 读取 |
| `QSPI_Flash_Write(addr, buf, len)` | 写入数据 |
| `QSPI_Flash_EraseSector(addr)` | 擦除扇区 (4KB) |
| `QSPI_Flash_EraseBlock(addr)` | 擦除块 (64KB) |
| `QSPI_Flash_EraseChip()` | 全片擦除 |
| `QSPI_EnableMemoryMapped()` | 使能内存映射模式 |

**常量**：`QSPI_FLASH_BASE`(0x90000000)、`W25Q_PAGE_SIZE`(256)、`W25Q_SECTOR_SIZE`(4096)、`W25Q_BLOCK_SIZE`(65536)  
需在工程中定义 `hqspi` 句柄。

---

## RF24L01 (RF24L01.h)

2.4GHz 无线收发，需在工程中定义 `RF24L01_CE_GPIO_Port` / `RF24L01_CE_Pin` 等引脚。

**主要接口**：`RF24L01_Init()`, `NRF24L01_TxPacket()`, `NRF24L01_RxPacket()`, `NRF24L01_Set_Mode()` 等。

---

## nanopb (pb/)

Protocol Buffers 编解码库，详见 `pb_encode.h`、`pb_decode.h`、`pb.h`。
