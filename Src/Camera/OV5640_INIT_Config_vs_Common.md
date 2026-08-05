# OV5640_INIT_Config 与 OV5640_Common 相同点与不同点

本文档对比 `ov5640.c` 中两种初始化配置表的异同。

---

## 一、相同点

### 1. 完全相同的寄存器配置（约 180 项）

以下模块在两表中**寄存器地址与取值均一致**：

| 模块 | 寄存器范围 | 说明 |
|------|------------|------|
| 模拟前端 | 0x3630~0x471c | 时序、增益、时钟等 |
| AEC 增益上限 | AEC_CTRL13, GAIN_CEILING | 0x43, 0x00f8 |
| 50/60Hz 滤波 | 5060HZ_CTRL01/04/05 | 0x34, 0x28, 0x98 |
| 采样数 | SAMPLE_NUMBER | 0x9c40 |
| 时序基础 | TIMING_HS/VS/HW/VH | 相同 |
| 预缩放偏移 | HOFFSET=0x10, VOFFSET 略有差异 |
| X/Y 增量 | TIMING_X_INC, Y_INC | 0x31, 0x31 |
| BLC 黑电平 | BLC_CTRL01, BLC_CTRL04 | 0x02, 0x02 |
| 系统复位 | SYSREM_RESET00/02 | 0x00, 0x1c |
| 时钟使能 | CLOCK_ENABLE00/02 | 0xff, 0xc3 |
| MIPI 控制 | MIPI_CONTROL00 | 0x58（禁用 MIPI，用 DVP） |
| 格式控制 | FORMAT_CTRL00, FORMAT_MUX_CTRL | 0x6F, 0x01 |
| ISP 控制 | ISP_CONTROL00/01 | 0xa7, 0xa3 |
| AWB 自动白平衡 | AWB_CTRL00~30 | 全部相同 |
| 色彩矩阵 CMX | CMX1~9, CMXSIGN | 全部相同 |
| CIP 锐化/降噪 | CIP_SHARPENMT/DNS 部分 | 部分相同 |
| Gamma | GAMMA_CTRL00, GAMMA_YST00~0F | 全部相同 |
| SDE | SDE_CTRL3/4/9/10/11 | 0x40, 0x10, 0x10, 0x00, 0xf8 |
| LENC 镜头补偿 | GMTRX00~55, BRMATRX, LENC_BR_OFFSET | 全部相同 |
| AEC 补偿 | AEC_CTRL0F/10/1B/1E/11/1F | 全部相同 |
| 唤醒 | SYSTEM_CTROL0 | 0x02 |

---

## 二、不同点

### 1. 开头与电源/复位

| 寄存器 | OV5640_INIT_Config | OV5640_Common |
|--------|---------------------|----------------|
| SCCB_SYSTEM_CTRL1 | 0x03（仅一次） | 0x11 → 0x03（先软件复位） |
| SYSTEM_CTROL0 | 0x42（掉电）→ 末尾 0x02（唤醒） | 0x82（软件复位）→ 末尾 0x02 |
| PAD_OUTPUT_ENABLE01 | 0xff | **无** |
| POLARITY_CTRL | 0x21 | 0x22 |
| PAD_OUTPUT_ENABLE02 | 0xff | **无** |

### 2. 时钟配置

| 项目 | OV5640_INIT_Config | OV5640_Common |
|------|---------------------|----------------|
| PLL 配置 | 有完整 PLL（SC_PLL_CONTRL0~3, ROOT_DIVIDER） | **无** |
| VFIFO_CTRL0C | 0x20 | 0x23 |
| TIMING_TC_REG24 | 0x02 | 0x02（相同） |
| PCLK_PERIOD | **无** | 0x22 |

### 3. 翻转/镜像

| 寄存器 | OV5640_INIT_Config | OV5640_Common |
|--------|---------------------|----------------|
| TIMING_TC_REG20 | 0x47（垂直翻转） | 0x06 |
| TIMING_TC_REG21 | 0x01（水平镜像） | 0x00 |

### 4. 窗口与输出尺寸

| 项目 | OV5640_INIT_Config | OV5640_Common |
|------|---------------------|----------------|
| 目标 | 4:3 1280×960 基础，由 SetResolution 设 400×300 | 直接 800×600 |
| TIMING_HTS/VTS | 0x0768, 0x03d8 | 0x0790, 0x0440 |
| TIMING_DVPHO/DVPVO | **无**（由 SetResolution 设置） | 0x0320, 0x0258 |
| VOFFSET | 0x04 | 0x06 |

### 5. BLC 与 AEC/曝光

| 寄存器 | OV5640_INIT_Config | OV5640_Common |
|--------|---------------------|----------------|
| BLC_CTRL05 | 0x1a | **无** |
| AEC_CTRL02 | 0x05 | 0x03 |
| AEC_CTRL03 | 0xc4 | 0xd8 |
| AEC_B50_STEP | 0x0093 | 0x0127 |
| AEC_B60_STEP | 0x007b | 0x00f6 |
| AEC_CTRL0D | 0x08 | 0x04 |
| AEC_CTRL0E | 0x06 | 0x03 |
| AEC_MAX_EXPO | 0x05c4 | 0x03d8 |

### 6. 测光阈值

| 寄存器 | OV5640_INIT_Config | OV5640_Common |
|--------|---------------------|----------------|
| LIGHTMETER1_TH | 0x0008 | 0x0000 |
| LIGHTMETER2_TH | 0x001c | 0x012c |

### 7. AWB 手动/增益

| 寄存器 | OV5640_INIT_Config | OV5640_Common |
|--------|---------------------|----------------|
| AWB_MANUAL_CONTROL | 0x00 | **无** |
| AWB_R/G/B_GAIN | 0x0400, 0x0400, 0x0400 | **无** |

### 8. CIP 锐化参数

| 寄存器 | OV5640_INIT_Config | OV5640_Common |
|--------|---------------------|----------------|
| CIP_SHARPENTH_TH1 | 0x08 | 0x30 |
| CIP_SHARPENTH_TH2 | 0x30 | 0x04 |
| CIP_SHARPENTH_OFFSET1 | 0x04 | 0x06 |
| CIP_SHARPENTH_OFFSET2 | 0x06 | **无**（被 CIP_CTRL 替代） |
| CIP_CTRL | **无** | 0x08 |

### 9. SDE 与 ISP 杂项

| 寄存器 | OV5640_INIT_Config | OV5640_Common |
|--------|---------------------|----------------|
| SDE_CTRL0 | 0x06 | 0x02 |
| ISP_MISC0 | 0x40 | **无** |

### 10. JPEG 相关（仅 Common）

| 寄存器 | OV5640_INIT_Config | OV5640_Common |
|--------|---------------------|----------------|
| JPG_MODE_SELECT | **无** | 0x03 |
| JPEG_CTRL07 | **无** | 0x04 |

### 11. 其他

| 寄存器 | OV5640_INIT_Config | OV5640_Common |
|--------|---------------------|----------------|
| 0x460b | 0x37 | 0x35 |
| 0x302e | 0x00 | 0x00（相同） |

---

## 三、配置顺序差异

- **INIT_Config**：先 PAD/POLARITY → 时钟 → 模拟前端 → 系统复位 → 格式/ISP → 翻转 → 窗口 → BLC → AEC → 50/60Hz → AWB → CMX → CIP → Gamma → SDE → AEC 补偿 → AWB 手动 → LENC → 唤醒
- **Common**：先软件复位 → 模拟前端 → 部分 AEC → 翻转 → 窗口（含 DVPHO/DVPVO）→ BLC → 系统复位 → POLARITY → 格式 → JPEG → VFIFO/PCLK → ISP → AWB → CMX → CIP → Gamma → SDE → LENC → AEC 补偿 → 唤醒

---

## 四、使用建议

| 场景 | 建议 |
|------|------|
| 240×240 小屏、鹿小班参考例程 | 定义 `USE_OV5640_REFERENCE_CONFIG`，使用 INIT_Config |
| 800×480 等大屏、ST 默认流程 | 不定义该宏，使用 OV5640_Common |
| 色彩异常、条纹 | 优先尝试 INIT_Config + DCMI X_Offset 为奇数 |
