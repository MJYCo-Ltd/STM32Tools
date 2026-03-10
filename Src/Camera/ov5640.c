/**
 ******************************************************************************
 * @file    ov5640.c
 * @author  MCD Application Team
 * @brief   本文件提供 OV5640 摄像头驱动
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2019-2020 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* 头文件包含
 * ------------------------------------------------------------------*/
#include <Camera/ov5640.h>
#ifdef USE_OV5640_REFERENCE_CONFIG
/// OV5640初始化配置
const uint16_t OV5640_INIT_Config[][2] = {
    {OV5640_SYSTEM_CTROL0, 0x42},     // 系统电源控制，Bit[6]设置为1进入掉电模式
    {OV5640_SCCB_SYSTEM_CTRL1, 0x03}, // 系统时钟选择，设置使用PLL生成后的时钟
    {OV5640_PAD_OUTPUT_ENABLE01,
     0xff},                       // PCLK、VS、HS以及数据引脚 D9~D6 使能输出
    {OV5640_POLARITY_CTRL, 0x21}, // 设置PCLK、VS和HS的信号极性
    {OV5640_PAD_OUTPUT_ENABLE02, 0xff}, // 数据引脚 D5~D0 使能输出

    /*------- 时钟配置，可结合资料里的《时钟图》进行参考
       -------------------------------------*/

    //	以下所有关于时钟的配置，都是在 XVCLK=24MHz 的前提下进行设置

    //	PLL预分频,bit[7:5]没有说明作用
    //	bit[4], PLL R
    // divider，用来设置是否将经过0x3035寄存器配置之后的时钟进行分频，设置为0不分频，为1则是2分频
    // Bit[3:0]，PLL pre-divider，预分频，此处设置为3，即将 XVCLK
    // 3分频后得到8M的时钟
    {OV5640_SC_PLL_CONTRL3, 0x13}, // 分频

    // Bit[7:0] 用于设置倍频参数，倍频值可以是4~127的任何数值
    {OV5640_SC_PLL_CONTRL2, 0x64}, // 倍频

    // Bit[3:0] 用于设置MIPI时钟，Bit[7:4] 分频系数
    {OV5640_SC_PLL_CONTRL1, 0x11}, // 分频

    // 0x3034的 Bit[3:0] 的取值会影响分频系数，此处设置是2.5分频，主时钟为 160M
    {OV5640_SC_PLL_CONTRL0, 0x1A}, // 手册的默认值

    // 设置 PCLK 和 SCLK 分频
    {OV5640_SYSTEM_ROOT_DIVIDER, 0x01}, // 分频

    // 0x460c Bit[7:4] JPEG空数据速度，Bit[1] PCLK分频控制
    {OV5640_VFIFO_CTRL0C, 0x20},

    // Bit[4:0]有效，只能设置为 1、2、4、8、16，PCLK分频系数
    {OV5640_TIMING_TC_REG24, 0x02}, // PCLK分频系数
                                    // fanke

    /*------------------------------------------------------------------
       时钟配置结束 -----*/

    // 手册里没有说明这些寄存器的作用,这里直接保留手册给的设置参数
    {0x3630, 0x36},
    {0x3631, 0x0e},
    {0x3632, 0xe2},
    {0x3633, 0x12},
    {0x3621, 0xe0},
    {0x3704, 0xa0}, // FanKe
    {0x3703, 0x5a},
    {0x3715, 0x78},
    {0x3717, 0x01},
    {0x370b, 0x60},
    {0x3705, 0x1a},
    {0x3905, 0x02},
    {0x3906, 0x10},
    {0x3901, 0x0a},
    {0x3731, 0x12},
    {0x3600, 0x08},
    {0x3601, 0x33},
    {0x302d, 0x60},
    {0x3620, 0x52},
    {0x371b, 0x20},
    {0x471c, 0x50},
    {0x3635, 0x13},
    {0x3636, 0x03},
    {0x3634, 0x40},
    {0x3622, 0x01},
    {0x440e, 0x00},
    {0x5025, 0x00},
    {0x3618, 0x00},
    {0x3612, 0x29},
    {0x3708, 0x64},
    {0x3709, 0x52},
    {0x370c, 0x03},
    {0x302e, 0x00},
    {0x460b, 0x37},

    {OV5640_SYSREM_RESET00, 0x00}, // 使能所有系统单元，包括 BIST、MCU、OTP等
    {OV5640_SYSREM_RESET02, 0x1c}, // 复位 JFIFO, SFIFO, JPG
    {OV5640_CLOCK_ENABLE00, 0xff}, // 使能所有系统单元
    {OV5640_CLOCK_ENABLE02, 0xc3}, // 禁止 JPEG2x, JPEG 的时钟
    {OV5640_MIPI_CONTROL00, 0x58}, // 禁止 MIPI，使用DVP接口（STM32的DCMI接口）

    // 设置数据接口输出的格式，RGB565格式，序列为 G[2:0]B[4:0], R[4:0]G[5:3]
    {OV5640_FORMAT_CTRL00, 0x6F},
    {OV5640_FORMAT_MUX_CTRL, 0x01}, // ISP格式，此处使用RGB格式
    {OV5640_ISP_CONTROL00, 0xa7}, // ISP设置，使能 LENC、黑色像素、白色像素、CIP
    {OV5640_ISP_CONTROL01,
     0xA3}, // ISP设置，使能 SDE、图像缩放、Color Matrix、AWB

    {OV5640_TIMING_TC_REG20, 0x47}, // Bit[2:1]垂直翻转
    {OV5640_TIMING_TC_REG21, 0x01}, // Bit[2:1]水平镜像，Bit[0]水平像素合并

    /*------- 窗口配置，参考OV5640数据手册 4.2 小节 image windowing
       ------------------------*/

    // OV5640有好几个窗口的概念
    // 摄像头的物理像素窗口：2624*1954
    // （包含黑电平矫正线和空像素，有效分辨率为2592*1944） ISP（image sensor
    // processor）输入窗口：需要进行处理的像素窗口
    // 预缩放窗口：ISP窗口的基础上，调整用于缩放输出的窗口
    // 输出窗口： 根据预缩放窗口和要输出的分辨率，进行缩放，得到最终的图像

    // 因为摄像头的默认像素比例是 2592/1944 = 4/3
    // （15帧），而我们实际使用的屏幕往往不是这个比例或者不需要这么高像素，
    // 因此需要做一定的调整，最终再配合DCMI的窗口裁剪以匹配屏幕。
    // （注：用户也可以直接使用OV5640裁剪 物理窗口 得到对应比例的
    // ISP窗口，然后缩放偏移按照默认设置即可， 例如可以直接将ISP窗口设置为
    // 240/280等对应实际屏幕的比例，而无需DCMI去裁剪。不过为了例程的通用性，我们选择
    // 使用 4:3固定比例+DCMI裁剪的方式	）

    // 以下配置为 4:3(1280*960) 43帧 的配置
    {OV5640_TIMING_HS_HIGH, 0x00},
    {OV5640_TIMING_HS_LOW, 0x00},
    {OV5640_TIMING_VS_HIGH, 0x00},
    {OV5640_TIMING_VS_LOW, 0x04},
    {OV5640_TIMING_HW_HIGH, 0x0a},
    {OV5640_TIMING_HW_LOW, 0x3f},
    {OV5640_TIMING_VH_HIGH, 0x07},
    {OV5640_TIMING_VH_LOW, 0x9b},
    {OV5640_TIMING_HTS_HIGH, 0x07},
    {OV5640_TIMING_HTS_LOW, 0x68},
    {OV5640_TIMING_VTS_HIGH, 0x03},
    {OV5640_TIMING_VTS_LOW, 0xd8},

    // 预缩放窗口，水平偏移16，垂直偏移4
    {OV5640_TIMING_HOFFSET_HIGH, 0x00},
    {OV5640_TIMING_HOFFSET_LOW, 0x10},
    {OV5640_TIMING_VOFFSET_HIGH, 0x00},
    {OV5640_TIMING_VOFFSET_LOW, 0x04},

    {OV5640_TIMING_X_INC, 0x31},
    {OV5640_TIMING_Y_INC, 0x31},

    /*------------------------------------------------------------------
       窗口配置结束 -----*/

    // BLC（Black Level Calibration ）黑电平校正
    {OV5640_BLC_CTRL01, 0x02},
    {OV5640_BLC_CTRL04, 0x02},
    {OV5640_BLC_CTRL05, 0x1a},

    // 曝光时间相关
    {OV5640_AEC_CTRL02, 0x05},
    {OV5640_AEC_CTRL03, 0xc4},
    {OV5640_AEC_B50_STEP_HIGH, 0x00},
    {OV5640_AEC_B50_STEP_LOW, 0x93},
    {OV5640_AEC_B60_STEP_HIGH, 0x00},
    {OV5640_AEC_B60_STEP_LOW, 0x7b},
    {OV5640_AEC_CTRL0D, 0x08},
    {OV5640_AEC_CTRL0E, 0x06},
    {OV5640_AEC_MAX_EXPO_HIGH, 0x05},
    {OV5640_AEC_MAX_EXPO_LOW, 0xc4},

    // AEC 增益相关
    {OV5640_AEC_CTRL13, 0x43},
    {OV5640_AEC_GAIN_CEILING_HIGH, 0x00},
    {OV5640_AEC_GAIN_CEILING_LOW, 0xf8},

    // 50/60Hz 灯光条纹过滤
    {OV5640_5060HZ_CTRL01, 0x34},
    {OV5640_5060HZ_CTRL04, 0x28},
    {OV5640_5060HZ_CTRL05, 0x98},
    {OV5640_LIGHTMETER1_TH_HIGH, 0x00},
    {OV5640_LIGHTMETER1_TH_LOW, 0x08},
    {OV5640_LIGHTMETER2_TH_HIGH, 0x00},
    {OV5640_LIGHTMETER2_TH_LOW, 0x1c},
    {OV5640_SAMPLE_NUMBER_HIGH, 0x9c},
    {OV5640_SAMPLE_NUMBER_LOW, 0x40},

    // AWB 自动白平衡
    {OV5640_AWB_CTRL00, 0xff},
    {OV5640_AWB_CTRL01, 0xf2},
    {OV5640_AWB_CTRL02, 0x00},
    {OV5640_AWB_CTRL03, 0x14},
    {OV5640_AWB_CTRL04, 0x25},
    {OV5640_AWB_CTRL05, 0x24},
    {OV5640_AWB_CTRL06, 0x09},
    {OV5640_AWB_CTRL07, 0x09},
    {OV5640_AWB_CTRL08, 0x09},
    {OV5640_AWB_CTRL09, 0x75},
    {OV5640_AWB_CTRL10, 0x54},
    {OV5640_AWB_CTRL11, 0xe0},
    {OV5640_AWB_CTRL12, 0xb2},
    {OV5640_AWB_CTRL13, 0x42},
    {OV5640_AWB_CTRL14, 0x3d},
    {OV5640_AWB_CTRL15, 0x56},
    {OV5640_AWB_CTRL16, 0x46},
    {OV5640_AWB_CTRL17, 0xf8},
    {OV5640_AWB_CTRL18, 0x04},
    {OV5640_AWB_CTRL19, 0x70},
    {OV5640_AWB_CTRL20, 0xf0},
    {OV5640_AWB_CTRL21, 0xf0},
    {OV5640_AWB_CTRL22, 0x03},
    {OV5640_AWB_CTRL23, 0x01},
    {OV5640_AWB_CTRL24, 0x04},
    {OV5640_AWB_CTRL25, 0x12},
    {OV5640_AWB_CTRL26, 0x04},
    {OV5640_AWB_CTRL27, 0x00},
    {OV5640_AWB_CTRL28, 0x06},
    {OV5640_AWB_CTRL29, 0x82},
    {OV5640_AWB_CTRL30, 0x38},

    // color matrix 色彩矩阵
    {OV5640_CMX1, 0x1e},
    {OV5640_CMX2, 0x5b},
    {OV5640_CMX3, 0x08},
    {OV5640_CMX4, 0x0a},
    {OV5640_CMX5, 0x7e},
    {OV5640_CMX6, 0x88},
    {OV5640_CMX7, 0x7c},
    {OV5640_CMX8, 0x6c},
    {OV5640_CMX9, 0x10},
    {OV5640_CMXSIGN_HIGH, 0x01},
    {OV5640_CMXSIGN_LOW, 0x98},

    // CIP 锐化和降噪
    {OV5640_CIP_SHARPENMT_TH1, 0x08},
    {OV5640_CIP_SHARPENMT_TH2, 0x30},
    {OV5640_CIP_SHARPENMT_OFFSET1, 0x10},
    {OV5640_CIP_SHARPENMT_OFFSET2, 0x00},
    {OV5640_CIP_DNS_TH1, 0x08},
    {OV5640_CIP_DNS_TH2, 0x30},
    {OV5640_CIP_DNS_OFFSET1, 0x08},
    {OV5640_CIP_DNS_OFFSET2, 0x16},
    {OV5640_CIP_SHARPENTH_TH1, 0x08},
    {OV5640_CIP_SHARPENTH_TH2, 0x30},
    {OV5640_CIP_SHARPENTH_OFFSET1, 0x04},
    {OV5640_CIP_SHARPENTH_OFFSET2, 0x06},

    // Gamma 伽玛曲线
    {OV5640_GAMMA_CTRL00, 0x01},
    {OV5640_GAMMA_YST00, 0x08},
    {OV5640_GAMMA_YST01, 0x14},
    {OV5640_GAMMA_YST02, 0x28},
    {OV5640_GAMMA_YST03, 0x51},
    {OV5640_GAMMA_YST04, 0x65},
    {OV5640_GAMMA_YST05, 0x71},
    {OV5640_GAMMA_YST06, 0x7d},
    {OV5640_GAMMA_YST07, 0x87},
    {OV5640_GAMMA_YST08, 0x91},
    {OV5640_GAMMA_YST09, 0x9a},
    {OV5640_GAMMA_YST0A, 0xaa},
    {OV5640_GAMMA_YST0B, 0xb8},
    {OV5640_GAMMA_YST0C, 0xcd},
    {OV5640_GAMMA_YST0D, 0xdd},
    {OV5640_GAMMA_YST0E, 0xea},
    {OV5640_GAMMA_YST0F, 0x1d},

    // UV adjust
    {OV5640_SDE_CTRL0, 0x06},
    {OV5640_SDE_CTRL3, 0x40},
    {OV5640_SDE_CTRL4, 0x10},
    {OV5640_SDE_CTRL9, 0x10},
    {OV5640_SDE_CTRL10, 0x00},
    {OV5640_SDE_CTRL11, 0xf8},
    {OV5640_ISP_MISC0, 0x40},

    // AEC 自动曝光补偿
    {OV5640_AEC_CTRL0F, 0x30},
    {OV5640_AEC_CTRL10, 0x28},
    {OV5640_AEC_CTRL1B, 0x30},
    {OV5640_AEC_CTRL1E, 0x26},
    {OV5640_AEC_CTRL11, 0x60},
    {OV5640_AEC_CTRL1F, 0x14},

    // AWB 环境光配置自动模式
    {OV5640_AWB_MANUAL_CONTROL, 0x00},
    {OV5640_AWB_R_GAIN_MSB, 0x04},
    {OV5640_AWB_R_GAIN_LSB, 0x00},
    {OV5640_AWB_G_GAIN_MSB, 0x04},
    {OV5640_AWB_G_GAIN_LSB, 0x00},
    {OV5640_AWB_B_GAIN_MSB, 0x04},
    {OV5640_AWB_B_GAIN_LSB, 0x00},

    // lens correction (LENC) 镜头补偿设置
    {OV5640_GMTRX00, 0x23},
    {OV5640_GMTRX01, 0x14},
    {OV5640_GMTRX02, 0x0f},
    {OV5640_GMTRX03, 0x0f},
    {OV5640_GMTRX04, 0x12},
    {OV5640_GMTRX05, 0x26},
    {OV5640_GMTRX10, 0x0c},
    {OV5640_GMTRX11, 0x08},
    {OV5640_GMTRX12, 0x05},
    {OV5640_GMTRX13, 0x05},
    {OV5640_GMTRX14, 0x08},
    {OV5640_GMTRX15, 0x0d},
    {OV5640_GMTRX20, 0x08},
    {OV5640_GMTRX21, 0x03},
    {OV5640_GMTRX22, 0x00},
    {OV5640_GMTRX23, 0x00},
    {OV5640_GMTRX24, 0x03},
    {OV5640_GMTRX25, 0x09},
    {OV5640_GMTRX30, 0x07},
    {OV5640_GMTRX31, 0x03},
    {OV5640_GMTRX32, 0x00},
    {OV5640_GMTRX33, 0x01},
    {OV5640_GMTRX34, 0x03},
    {OV5640_GMTRX35, 0x08},
    {OV5640_GMTRX40, 0x0d},
    {OV5640_GMTRX41, 0x08},
    {OV5640_GMTRX42, 0x05},
    {OV5640_GMTRX43, 0x06},
    {OV5640_GMTRX44, 0x08},
    {OV5640_GMTRX45, 0x0e},
    {OV5640_GMTRX50, 0x29},
    {OV5640_GMTRX51, 0x17},
    {OV5640_GMTRX52, 0x11},
    {OV5640_GMTRX53, 0x11},
    {OV5640_GMTRX54, 0x15},
    {OV5640_GMTRX55, 0x28},
    {OV5640_BRMATRX00, 0x46},
    {OV5640_BRMATRX01, 0x26},
    {OV5640_BRMATRX02, 0x08},
    {OV5640_BRMATRX03, 0x26},
    {OV5640_BRMATRX04, 0x64},
    {OV5640_BRMATRX05, 0x26},
    {OV5640_BRMATRX06, 0x24},
    {OV5640_BRMATRX07, 0x22},
    {OV5640_BRMATRX08, 0x24},
    {OV5640_BRMATRX09, 0x24},
    {OV5640_BRMATRX20, 0x06},
    {OV5640_BRMATRX21, 0x22},
    {OV5640_BRMATRX22, 0x40},
    {OV5640_BRMATRX23, 0x42},
    {OV5640_BRMATRX24, 0x24},
    {OV5640_BRMATRX30, 0x26},
    {OV5640_BRMATRX31, 0x24},
    {OV5640_BRMATRX32, 0x22},
    {OV5640_BRMATRX33, 0x22},
    {OV5640_BRMATRX34, 0x26},
    {OV5640_BRMATRX40, 0x44},
    {OV5640_BRMATRX41, 0x24},
    {OV5640_BRMATRX42, 0x26},
    {OV5640_BRMATRX43, 0x28},
    {OV5640_BRMATRX44, 0x42},
    {OV5640_LENC_BR_OFFSET, 0xce},

    // 系统电源控制，从掉电模式中唤醒
    {OV5640_SYSTEM_CTROL0, 0x02}};
#endif

/** @addtogroup BSP
 * @{
 */

/** @addtogroup Components
 * @{
 */

/** @addtogroup OV5640
 * @brief     本文件提供驱动 OV5640 摄像头模块所需的一组函数。
 * @{
 */

/** @defgroup OV5640_Private_TypesDefinitions
 * @{
 */

/**
 * @}
 */

/**
 * @}
 */

/** @defgroup OV5640_CAMERA_Drv_Prototypes OV5640_CAMERA_Driver 函数原型（从 ov5640.h 移至此处）
 * @{
 */
int32_t OV5640_Init(OV5640_Object_t *pObj, uint32_t Resolution, uint32_t PixelFormat);
int32_t OV5640_DeInit(OV5640_Object_t *pObj);
int32_t OV5640_ReadID(OV5640_Object_t *pObj, uint32_t *Id);
int32_t OV5640_GetCapabilities(OV5640_Object_t *pObj, OV5640_Capabilities_t *Capabilities);
int32_t OV5640_SetLightMode(OV5640_Object_t *pObj, uint32_t LightMode);
int32_t OV5640_SetColorEffect(OV5640_Object_t *pObj, uint32_t Effect);
int32_t OV5640_SetBrightness(OV5640_Object_t *pObj, int32_t Level);
int32_t OV5640_SetSaturation(OV5640_Object_t *pObj, int32_t Level);
int32_t OV5640_SetContrast(OV5640_Object_t *pObj, int32_t Level);
int32_t OV5640_SetHueDegree(OV5640_Object_t *pObj, int32_t Degree);
int32_t OV5640_MirrorFlipConfig(OV5640_Object_t *pObj, uint32_t Config);
int32_t OV5640_ZoomConfig(OV5640_Object_t *pObj, uint32_t Zoom);
int32_t OV5640_SetResolution(OV5640_Object_t *pObj, uint32_t Resolution);
int32_t OV5640_GetResolution(OV5640_Object_t *pObj, uint32_t *Resolution);
int32_t OV5640_SetPixelFormat(OV5640_Object_t *pObj, uint32_t PixelFormat);
int32_t OV5640_GetPixelFormat(OV5640_Object_t *pObj, uint32_t *PixelFormat);
int32_t OV5640_NightModeConfig(OV5640_Object_t *pObj, uint32_t Cmd);
/**
 * @}
 */

/** @defgroup OV5640_Private_Variables
 * @{
 */

OV5640_CAMERA_Drv_t OV5640_CAMERA_Driver = {OV5640_Init,
                                            OV5640_DeInit,
                                            OV5640_ReadID,
                                            OV5640_GetCapabilities,
                                            OV5640_SetLightMode,
                                            OV5640_SetColorEffect,
                                            OV5640_SetBrightness,
                                            OV5640_SetSaturation,
                                            OV5640_SetContrast,
                                            OV5640_SetHueDegree,
                                            OV5640_MirrorFlipConfig,
                                            OV5640_ZoomConfig,
                                            OV5640_SetResolution,
                                            OV5640_GetResolution,
                                            OV5640_SetPixelFormat,
                                            OV5640_GetPixelFormat,
                                            OV5640_NightModeConfig};

/** @defgroup OV5640_Private_Functions_Prototypes 私有函数原型
 * @{
 */
static int32_t OV5640_ReadRegWrap(void *handle, uint16_t Reg, uint8_t *Data,
                                  uint16_t Length);
static int32_t OV5640_WriteRegWrap(void *handle, uint16_t Reg, uint8_t *Data,
                                   uint16_t Length);
static int32_t OV5640_Delay(OV5640_Object_t *pObj, uint32_t Delay);

/**
 * @}
 */

/** @defgroup OV5640_Exported_Functions OV5640 Exported Functions
 * @{
 */
/**
 * @brief  注册组件 IO 总线
 * @param  pObj  组件对象指针
 * @retval 组件状态
 */
int32_t OV5640_RegisterBusIO(OV5640_Object_t *pObj, OV5640_IO_t *pIO) {
  int32_t ret;

  if (pObj == NULL) {
    ret = OV5640_ERROR;
  } else {
    pObj->IO.Init = pIO->Init;
    pObj->IO.DeInit = pIO->DeInit;
    pObj->IO.Address = pIO->Address;
    pObj->IO.WriteReg = pIO->WriteReg;
    pObj->IO.ReadReg = pIO->ReadReg;
    pObj->IO.GetTick = pIO->GetTick;

    pObj->Ctx.ReadReg = OV5640_ReadRegWrap;
    pObj->Ctx.WriteReg = OV5640_WriteRegWrap;
    pObj->Ctx.handle = pObj;

    if (pObj->IO.Init != NULL) {
      ret = pObj->IO.Init();
    } else {
      ret = OV5640_ERROR;
    }
  }

  return ret;
}

/**
 * @brief  初始化 OV5640 摄像头组件。
 * @param  pObj  组件对象指针
 * @param  Resolution  摄像头分辨率
 * @param  PixelFormat 待配置的像素格式
 * @retval 组件状态
 */
int32_t OV5640_Init(OV5640_Object_t *pObj, uint32_t Resolution,
                    uint32_t PixelFormat) {
  uint32_t index;
  int32_t ret = OV5640_OK;

  /* OV5640 初始化序列 */
  static const uint16_t OV5640_Common[][2] = {
      {OV5640_SCCB_SYSTEM_CTRL1, 0x11},
      {OV5640_SYSTEM_CTROL0, 0x82},
      {OV5640_SCCB_SYSTEM_CTRL1, 0x03},
      {0x3630, 0x36},
      {0x3631, 0x0e},
      {0x3632, 0xe2},
      {0x3633, 0x12},
      {0x3621, 0xe0},
      {0x3704, 0xa0},
      {0x3703, 0x5a},
      {0x3715, 0x78},
      {0x3717, 0x01},
      {0x370b, 0x60},
      {0x3705, 0x1a},
      {0x3905, 0x02},
      {0x3906, 0x10},
      {0x3901, 0x0a},
      {0x3731, 0x12},
      {0x3600, 0x08},
      {0x3601, 0x33},
      {0x302d, 0x60},
      {0x3620, 0x52},
      {0x371b, 0x20},
      {0x471c, 0x50},
      {OV5640_AEC_CTRL13, 0x43},
      {OV5640_AEC_GAIN_CEILING_HIGH, 0x00},
      {OV5640_AEC_GAIN_CEILING_LOW, 0xf8},
      {0x3635, 0x13},
      {0x3636, 0x03},
      {0x3634, 0x40},
      {0x3622, 0x01},
      {OV5640_5060HZ_CTRL01, 0x34},
      {OV5640_5060HZ_CTRL04, 0x28},
      {OV5640_5060HZ_CTRL05, 0x98},
      {OV5640_LIGHTMETER1_TH_HIGH, 0x00},
      {OV5640_LIGHTMETER1_TH_LOW, 0x00},
      {OV5640_LIGHTMETER2_TH_HIGH, 0x01},
      {OV5640_LIGHTMETER2_TH_LOW, 0x2c},
      {OV5640_SAMPLE_NUMBER_HIGH, 0x9c},
      {OV5640_SAMPLE_NUMBER_LOW, 0x40},
      {OV5640_TIMING_TC_REG20, 0x06},
      {OV5640_TIMING_TC_REG21, 0x00},
      {OV5640_TIMING_X_INC, 0x31},
      {OV5640_TIMING_Y_INC, 0x31},
      {OV5640_TIMING_HS_HIGH, 0x00},
      {OV5640_TIMING_HS_LOW, 0x00},
      {OV5640_TIMING_VS_HIGH, 0x00},
      {OV5640_TIMING_VS_LOW, 0x04},
      {OV5640_TIMING_HW_HIGH, 0x0a},
      {OV5640_TIMING_HW_LOW, 0x3f},
      {OV5640_TIMING_VH_HIGH, 0x07},
      {OV5640_TIMING_VH_LOW, 0x9b},
      {OV5640_TIMING_DVPHO_HIGH, 0x03},
      {OV5640_TIMING_DVPHO_LOW, 0x20},
      {OV5640_TIMING_DVPVO_HIGH, 0x02},
      {OV5640_TIMING_DVPVO_LOW, 0x58},
      /* 800x480 分辨率: OV5640_TIMING_HTS=0x790, OV5640_TIMING_VTS=0x440 */
      {OV5640_TIMING_HTS_HIGH, 0x07},
      {OV5640_TIMING_HTS_LOW, 0x90},
      {OV5640_TIMING_VTS_HIGH, 0x04},
      {OV5640_TIMING_VTS_LOW, 0x40},
      {OV5640_TIMING_HOFFSET_HIGH, 0x00},
      {OV5640_TIMING_HOFFSET_LOW, 0x10},
      {OV5640_TIMING_VOFFSET_HIGH, 0x00},
      {OV5640_TIMING_VOFFSET_LOW, 0x06},
      {0x3618, 0x00},
      {0x3612, 0x29},
      {0x3708, 0x64},
      {0x3709, 0x52},
      {0x370c, 0x03},
      {OV5640_AEC_CTRL02, 0x03},
      {OV5640_AEC_CTRL03, 0xd8},
      {OV5640_AEC_B50_STEP_HIGH, 0x01},
      {OV5640_AEC_B50_STEP_LOW, 0x27},
      {OV5640_AEC_B60_STEP_HIGH, 0x00},
      {OV5640_AEC_B60_STEP_LOW, 0xf6},
      {OV5640_AEC_CTRL0E, 0x03},
      {OV5640_AEC_CTRL0D, 0x04},
      {OV5640_AEC_MAX_EXPO_HIGH, 0x03},
      {OV5640_AEC_MAX_EXPO_LOW, 0xd8},
      {OV5640_BLC_CTRL01, 0x02},
      {OV5640_BLC_CTRL04, 0x02},
      {OV5640_SYSREM_RESET00, 0x00},
      {OV5640_SYSREM_RESET02, 0x1c},
      {OV5640_CLOCK_ENABLE00, 0xff},
      {OV5640_CLOCK_ENABLE02, 0xc3},
      {OV5640_MIPI_CONTROL00, 0x58},
      {0x302e, 0x00},
      {OV5640_POLARITY_CTRL, 0x22},
      {OV5640_FORMAT_CTRL00, 0x6F},
      {OV5640_FORMAT_MUX_CTRL, 0x01},
      {OV5640_JPG_MODE_SELECT, 0x03},
      {OV5640_JPEG_CTRL07, 0x04},
      {0x440e, 0x00},
      {0x460b, 0x35},
      {OV5640_VFIFO_CTRL0C, 0x23},
      {OV5640_PCLK_PERIOD, 0x22},
      {OV5640_TIMING_TC_REG24, 0x02},
      {OV5640_ISP_CONTROL00, 0xa7},
      {OV5640_ISP_CONTROL01, 0xa3},
      {OV5640_AWB_CTRL00, 0xff},
      {OV5640_AWB_CTRL01, 0xf2},
      {OV5640_AWB_CTRL02, 0x00},
      {OV5640_AWB_CTRL03, 0x14},
      {OV5640_AWB_CTRL04, 0x25},
      {OV5640_AWB_CTRL05, 0x24},
      {OV5640_AWB_CTRL06, 0x09},
      {OV5640_AWB_CTRL07, 0x09},
      {OV5640_AWB_CTRL08, 0x09},
      {OV5640_AWB_CTRL09, 0x75},
      {OV5640_AWB_CTRL10, 0x54},
      {OV5640_AWB_CTRL11, 0xe0},
      {OV5640_AWB_CTRL12, 0xb2},
      {OV5640_AWB_CTRL13, 0x42},
      {OV5640_AWB_CTRL14, 0x3d},
      {OV5640_AWB_CTRL15, 0x56},
      {OV5640_AWB_CTRL16, 0x46},
      {OV5640_AWB_CTRL17, 0xf8},
      {OV5640_AWB_CTRL18, 0x04},
      {OV5640_AWB_CTRL19, 0x70},
      {OV5640_AWB_CTRL20, 0xf0},
      {OV5640_AWB_CTRL21, 0xf0},
      {OV5640_AWB_CTRL22, 0x03},
      {OV5640_AWB_CTRL23, 0x01},
      {OV5640_AWB_CTRL24, 0x04},
      {OV5640_AWB_CTRL25, 0x12},
      {OV5640_AWB_CTRL26, 0x04},
      {OV5640_AWB_CTRL27, 0x00},
      {OV5640_AWB_CTRL28, 0x06},
      {OV5640_AWB_CTRL29, 0x82},
      {OV5640_AWB_CTRL30, 0x38},
      {OV5640_CMX1, 0x1e},
      {OV5640_CMX2, 0x5b},
      {OV5640_CMX3, 0x08},
      {OV5640_CMX4, 0x0a},
      {OV5640_CMX5, 0x7e},
      {OV5640_CMX6, 0x88},
      {OV5640_CMX7, 0x7c},
      {OV5640_CMX8, 0x6c},
      {OV5640_CMX9, 0x10},
      {OV5640_CMXSIGN_HIGH, 0x01},
      {OV5640_CMXSIGN_LOW, 0x98},
      {OV5640_CIP_SHARPENMT_TH1, 0x08},
      {OV5640_CIP_SHARPENMT_TH2, 0x30},
      {OV5640_CIP_SHARPENMT_OFFSET1, 0x10},
      {OV5640_CIP_SHARPENMT_OFFSET2, 0x00},
      {OV5640_CIP_DNS_TH1, 0x08},
      {OV5640_CIP_DNS_TH2, 0x30},
      {OV5640_CIP_DNS_OFFSET1, 0x08},
      {OV5640_CIP_DNS_OFFSET2, 0x16},
      {OV5640_CIP_CTRL, 0x08},
      {OV5640_CIP_SHARPENTH_TH1, 0x30},
      {OV5640_CIP_SHARPENTH_TH2, 0x04},
      {OV5640_CIP_SHARPENTH_OFFSET1, 0x06},
      {OV5640_GAMMA_CTRL00, 0x01},
      {OV5640_GAMMA_YST00, 0x08},
      {OV5640_GAMMA_YST01, 0x14},
      {OV5640_GAMMA_YST02, 0x28},
      {OV5640_GAMMA_YST03, 0x51},
      {OV5640_GAMMA_YST04, 0x65},
      {OV5640_GAMMA_YST05, 0x71},
      {OV5640_GAMMA_YST06, 0x7d},
      {OV5640_GAMMA_YST07, 0x87},
      {OV5640_GAMMA_YST08, 0x91},
      {OV5640_GAMMA_YST09, 0x9a},
      {OV5640_GAMMA_YST0A, 0xaa},
      {OV5640_GAMMA_YST0B, 0xb8},
      {OV5640_GAMMA_YST0C, 0xcd},
      {OV5640_GAMMA_YST0D, 0xdd},
      {OV5640_GAMMA_YST0E, 0xea},
      {OV5640_GAMMA_YST0F, 0x1d},
      {OV5640_SDE_CTRL0, 0x02},
      {OV5640_SDE_CTRL3, 0x40},
      {OV5640_SDE_CTRL4, 0x10},
      {OV5640_SDE_CTRL9, 0x10},
      {OV5640_SDE_CTRL10, 0x00},
      {OV5640_SDE_CTRL11, 0xf8},
      {OV5640_GMTRX00, 0x23},
      {OV5640_GMTRX01, 0x14},
      {OV5640_GMTRX02, 0x0f},
      {OV5640_GMTRX03, 0x0f},
      {OV5640_GMTRX04, 0x12},
      {OV5640_GMTRX05, 0x26},
      {OV5640_GMTRX10, 0x0c},
      {OV5640_GMTRX11, 0x08},
      {OV5640_GMTRX12, 0x05},
      {OV5640_GMTRX13, 0x05},
      {OV5640_GMTRX14, 0x08},
      {OV5640_GMTRX15, 0x0d},
      {OV5640_GMTRX20, 0x08},
      {OV5640_GMTRX21, 0x03},
      {OV5640_GMTRX22, 0x00},
      {OV5640_GMTRX23, 0x00},
      {OV5640_GMTRX24, 0x03},
      {OV5640_GMTRX25, 0x09},
      {OV5640_GMTRX30, 0x07},
      {OV5640_GMTRX31, 0x03},
      {OV5640_GMTRX32, 0x00},
      {OV5640_GMTRX33, 0x01},
      {OV5640_GMTRX34, 0x03},
      {OV5640_GMTRX35, 0x08},
      {OV5640_GMTRX40, 0x0d},
      {OV5640_GMTRX41, 0x08},
      {OV5640_GMTRX42, 0x05},
      {OV5640_GMTRX43, 0x06},
      {OV5640_GMTRX44, 0x08},
      {OV5640_GMTRX45, 0x0e},
      {OV5640_GMTRX50, 0x29},
      {OV5640_GMTRX51, 0x17},
      {OV5640_GMTRX52, 0x11},
      {OV5640_GMTRX53, 0x11},
      {OV5640_GMTRX54, 0x15},
      {OV5640_GMTRX55, 0x28},
      {OV5640_BRMATRX00, 0x46},
      {OV5640_BRMATRX01, 0x26},
      {OV5640_BRMATRX02, 0x08},
      {OV5640_BRMATRX03, 0x26},
      {OV5640_BRMATRX04, 0x64},
      {OV5640_BRMATRX05, 0x26},
      {OV5640_BRMATRX06, 0x24},
      {OV5640_BRMATRX07, 0x22},
      {OV5640_BRMATRX08, 0x24},
      {OV5640_BRMATRX09, 0x24},
      {OV5640_BRMATRX20, 0x06},
      {OV5640_BRMATRX21, 0x22},
      {OV5640_BRMATRX22, 0x40},
      {OV5640_BRMATRX23, 0x42},
      {OV5640_BRMATRX24, 0x24},
      {OV5640_BRMATRX30, 0x26},
      {OV5640_BRMATRX31, 0x24},
      {OV5640_BRMATRX32, 0x22},
      {OV5640_BRMATRX33, 0x22},
      {OV5640_BRMATRX34, 0x26},
      {OV5640_BRMATRX40, 0x44},
      {OV5640_BRMATRX41, 0x24},
      {OV5640_BRMATRX42, 0x26},
      {OV5640_BRMATRX43, 0x28},
      {OV5640_BRMATRX44, 0x42},
      {OV5640_LENC_BR_OFFSET, 0xce},
      {0x5025, 0x00},
      {OV5640_AEC_CTRL0F, 0x30},
      {OV5640_AEC_CTRL10, 0x28},
      {OV5640_AEC_CTRL1B, 0x30},
      {OV5640_AEC_CTRL1E, 0x26},
      {OV5640_AEC_CTRL11, 0x60},
      {OV5640_AEC_CTRL1F, 0x14},
      {OV5640_SYSTEM_CTROL0, 0x02},
  };
  uint8_t tmp;

  if (pObj->IsInitialized == 0U) {
    /* 检查分辨率是否支持 */
    if ((Resolution > OV5640_R400x300) ||
        ((PixelFormat != OV5640_RGB565) && (PixelFormat != OV5640_YUV422) &&
         (PixelFormat != OV5640_RGB888) && (PixelFormat != OV5640_Y8) &&
         (PixelFormat != OV5640_JPEG))) {
      ret = OV5640_ERROR;
    } else {
      /* 为所有分辨率设置通用参数 */
#ifdef USE_OV5640_REFERENCE_CONFIG
      /* 使用参考例程（鹿小班 240x240 屏幕）的 OV5640_INIT_Config */
      for (index = 0;
           index < (sizeof(OV5640_INIT_Config) / sizeof(OV5640_INIT_Config[0]));
           index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_INIT_Config[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_INIT_Config[index][0], &tmp,
                               1) != OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
#else
      for (index = 0; index < (sizeof(OV5640_Common) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_Common[index][1];

          if (ov5640_write_reg(&pObj->Ctx, OV5640_Common[index][0], &tmp, 1) !=
              OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
#endif

      if (ret == OV5640_OK) {
        /* 设置串行接口配置 */
        if (pObj->Mode == SERIAL_MODE) {
          if (OV5640_EnableMIPIMode(pObj) != OV5640_OK) {
            ret = OV5640_ERROR;
          } else if (OV5640_SetMIPIVirtualChannel(
                         pObj, pObj->VirtualChannelID) != OV5640_OK) {
            ret = OV5640_ERROR;
          }
        } else {
          /* 设置并行接口配置 */
          if (OV5640_EnableDVPMode(pObj) != OV5640_OK) {
            ret = OV5640_ERROR;
          } else {
            ret = OV5640_OK;
          }
        }
      }

      if (ret == OV5640_OK) {
        /* 为每种分辨率设置特定参数 */
        if (OV5640_SetResolution(pObj, Resolution) != OV5640_OK) {
          ret = OV5640_ERROR;
        } /* 为每种像素格式设置特定参数 */
        else if (OV5640_SetPixelFormat(pObj, PixelFormat) != OV5640_OK) {
          ret = OV5640_ERROR;
        } /* 设置像素时钟、Href 和 VSync 极性 */
        else if (OV5640_SetPolarities(pObj, OV5640_POLARITY_PCLK_HIGH,
                                      OV5640_POLARITY_HREF_LOW,
                                      OV5640_POLARITY_VSYNC_LOW) != OV5640_OK) {
          ret = OV5640_ERROR;
        } else {
          pObj->IsInitialized = 1U;
        }
      }
    }
  }

  return ret;
}

/**
 * @brief  反初始化摄像头传感器。
 * @param  pObj  组件对象指针
 * @retval 组件状态
 */
int32_t OV5640_DeInit(OV5640_Object_t *pObj) {
  if (pObj->IsInitialized == 1U) {
    /* 反初始化摄像头传感器接口 */
    pObj->IsInitialized = 0U;
  }

  return OV5640_OK;
}

/**
 * @brief  设置 OV5640 摄像头像素格式。
 * @param  pObj  组件对象指针
 * @param  PixelFormat 待配置的像素格式
 * @retval 组件状态
 */
int32_t OV5640_SetPixelFormat(OV5640_Object_t *pObj, uint32_t PixelFormat) {
  int32_t ret = OV5640_OK;
  uint32_t index;
  uint8_t tmp;

  /* RGB565 像素格式初始化序列 */
  static const uint16_t OV5640_PF_RGB565[][2] = {
      /*  设置像素格式: RGB565 */
      {OV5640_FORMAT_CTRL00, 0x6F},
      {OV5640_FORMAT_MUX_CTRL, 0x01},
  };

  /* YUV422 像素格式初始化序列 */
  static const uint16_t OV5640_PF_YUV422[][2] = {
      /*  设置像素格式: YUV422 */
      {OV5640_FORMAT_CTRL00, 0x30},
      {OV5640_FORMAT_MUX_CTRL, 0x00},
  };

  /* RGB888 像素格式初始化序列 */
  static const uint16_t OV5640_PF_RGB888[][2] = {
      /*  设置像素格式: RGB888 (RGBRGB)*/
      {OV5640_FORMAT_CTRL00, 0x23},
      {OV5640_FORMAT_MUX_CTRL, 0x01},
  };

  /* 8 位单色像素格式初始化序列 */
  static const uint16_t OV5640_PF_Y8[][2] = {
      /*  设置像素格式: Y 8 位 */
      {OV5640_FORMAT_CTRL00, 0x10},
      {OV5640_FORMAT_MUX_CTRL, 0x00},
  };

  /* JPEG 格式初始化序列 */
  static const uint16_t OV5640_PF_JPEG[][2] = {
      /*  设置像素格式: JPEG */
      {OV5640_FORMAT_CTRL00, 0x30},
      {OV5640_FORMAT_MUX_CTRL, 0x00},
  };

  /* 检查像素格式是否支持 */
  if ((PixelFormat != OV5640_RGB565) && (PixelFormat != OV5640_YUV422) &&
      (PixelFormat != OV5640_RGB888) && (PixelFormat != OV5640_Y8) &&
      (PixelFormat != OV5640_JPEG)) {
    /* 不支持的像素格式 */
    ret = OV5640_ERROR;
  } else {
    /* 为每种像素格式设置特定参数 */
    switch (PixelFormat) {
    case OV5640_YUV422:
      for (index = 0; index < (sizeof(OV5640_PF_YUV422) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_PF_YUV422[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_PF_YUV422[index][0], &tmp,
                               1) != OV5640_OK) {
            ret = OV5640_ERROR;
          } else {
            (void)OV5640_Delay(pObj, 1);
          }
        }
      }
      break;

    case OV5640_RGB888:
      for (index = 0; index < (sizeof(OV5640_PF_RGB888) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_PF_RGB888[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_PF_RGB888[index][0], &tmp,
                               1) != OV5640_OK) {
            ret = OV5640_ERROR;
          } else {
            (void)OV5640_Delay(pObj, 1);
          }
        }
      }
      break;

    case OV5640_Y8:
      for (index = 0; index < (sizeof(OV5640_PF_Y8) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_PF_Y8[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_PF_Y8[index][0], &tmp, 1) !=
              OV5640_OK) {
            ret = OV5640_ERROR;
          } else {
            (void)OV5640_Delay(pObj, 1);
          }
        }
      }
      break;

    case OV5640_JPEG:
      for (index = 0; index < (sizeof(OV5640_PF_JPEG) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_PF_JPEG[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_PF_JPEG[index][0], &tmp, 1) !=
              OV5640_OK) {
            ret = OV5640_ERROR;
          } else {
            (void)OV5640_Delay(pObj, 1);
          }
        }
      }
      break;

    case OV5640_RGB565:
    default:
      for (index = 0; index < (sizeof(OV5640_PF_RGB565) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_PF_RGB565[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_PF_RGB565[index][0], &tmp,
                               1) != OV5640_OK) {
            ret = OV5640_ERROR;
          } else {
            (void)OV5640_Delay(pObj, 1);
          }
        }
      }
      /* 与参考例程 OV5640_Set_Pixformat 一致：RGB565 时配置
       * 0x3821/0x3002/0x3006 */
      if (ret == OV5640_OK) {
        if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp, 1) ==
            OV5640_OK) {
          tmp = (uint8_t)((tmp & 0xDFU) | 0x00U); /* Bit[5]=0 非 JPEG 模式 */
          (void)ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp, 1);
        }
        if (ov5640_read_reg(&pObj->Ctx, OV5640_SYSREM_RESET02, &tmp, 1) ==
            OV5640_OK) {
          tmp = (uint8_t)((tmp & 0xE3U) | 0x1CU); /* 使能 VFIFO、SFIFO、JPG */
          (void)ov5640_write_reg(&pObj->Ctx, OV5640_SYSREM_RESET02, &tmp, 1);
        }
        if (ov5640_read_reg(&pObj->Ctx, OV5640_CLOCK_ENABLE02, &tmp, 1) ==
            OV5640_OK) {
          tmp = (uint8_t)((tmp & 0xD7U) | 0x00U); /* 禁止 JPEG 时钟 */
          (void)ov5640_write_reg(&pObj->Ctx, OV5640_CLOCK_ENABLE02, &tmp, 1);
        }
      }
      break;
    }

    if (PixelFormat == OV5640_JPEG) {
      if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp, 1) !=
          OV5640_OK) {
        ret = OV5640_ERROR;
      } else {
        tmp |= (1 << 5);
        if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp, 1) !=
            OV5640_OK) {
          ret = OV5640_ERROR;
        } else {
          if (ov5640_read_reg(&pObj->Ctx, OV5640_SYSREM_RESET02, &tmp, 1) !=
              OV5640_OK) {
            ret = OV5640_ERROR;
          } else {
            tmp &= ~((1 << 4) | (1 << 3) | (1 << 2));
            if (ov5640_write_reg(&pObj->Ctx, OV5640_SYSREM_RESET02, &tmp, 1) !=
                OV5640_OK) {
              ret = OV5640_ERROR;
            } else {
              if (ov5640_read_reg(&pObj->Ctx, OV5640_CLOCK_ENABLE02, &tmp, 1) !=
                  OV5640_OK) {
                ret = OV5640_ERROR;
              } else {
                tmp |= ((1 << 5) | (1 << 3));
                if (ov5640_write_reg(&pObj->Ctx, OV5640_CLOCK_ENABLE02, &tmp,
                                     1) != OV5640_OK) {
                  ret = OV5640_ERROR;
                }
              }
            }
          }
        }
      }
    }
  }
  return ret;
}

/**
 * @brief  获取 OV5640 摄像头像素格式。
 * @param  pObj  组件对象指针
 * @param  PixelFormat 像素格式指针
 * @retval 组件状态
 */
int32_t OV5640_GetPixelFormat(OV5640_Object_t *pObj, uint32_t *PixelFormat) {
  (void)(pObj);
  (void)(PixelFormat);

  return OV5640_ERROR;
}

/**
 * @brief  设置 OV5640 摄像头分辨率。
 * @param  pObj  组件对象指针
 * @param  Resolution  摄像头分辨率
 * @retval 组件状态
 */
int32_t OV5640_SetResolution(OV5640_Object_t *pObj, uint32_t Resolution) {
  int32_t ret = OV5640_OK;
  uint32_t index;
  uint8_t tmp;

  /* WVGA 分辨率 (800x480) 初始化序列 */
  static const uint16_t OV5640_WVGA[][2] = {
      {OV5640_TIMING_DVPHO_HIGH, 0x03},
      {OV5640_TIMING_DVPHO_LOW, 0x20},
      {OV5640_TIMING_DVPVO_HIGH, 0x01},
      {OV5640_TIMING_DVPVO_LOW, 0xE0},
  };

  /* VGA 分辨率 (640x480) 初始化序列 */
  static const uint16_t OV5640_VGA[][2] = {
      {OV5640_TIMING_DVPHO_HIGH, 0x02},
      {OV5640_TIMING_DVPHO_LOW, 0x80},
      {OV5640_TIMING_DVPVO_HIGH, 0x01},
      {OV5640_TIMING_DVPVO_LOW, 0xE0},
  };

  /* 480x272 分辨率初始化序列 */
  static const uint16_t OV5640_480x272[][2] = {
      {OV5640_TIMING_DVPHO_HIGH, 0x01},
      {OV5640_TIMING_DVPHO_LOW, 0xE0},
      {OV5640_TIMING_DVPVO_HIGH, 0x01},
      {OV5640_TIMING_DVPVO_LOW, 0x10},
  };

  /* 400x300 分辨率（鹿小班参考例程 4:3）初始化序列 */
  static const uint16_t OV5640_400x300[][2] = {
      {OV5640_TIMING_DVPHO_HIGH, 0x01},
      {OV5640_TIMING_DVPHO_LOW, 0x90},
      {OV5640_TIMING_DVPVO_HIGH, 0x01},
      {OV5640_TIMING_DVPVO_LOW, 0x2C},
  };

  /* QVGA 分辨率 (320x240) 初始化序列 */
  static const uint16_t OV5640_QVGA[][2] = {
      {OV5640_TIMING_DVPHO_HIGH, 0x01},
      {OV5640_TIMING_DVPHO_LOW, 0x40},
      {OV5640_TIMING_DVPVO_HIGH, 0x00},
      {OV5640_TIMING_DVPVO_LOW, 0xF0},
  };

  /* QQVGA 分辨率 (160x120) 初始化序列 */
  static const uint16_t OV5640_QQVGA[][2] = {
      {OV5640_TIMING_DVPHO_HIGH, 0x00},
      {OV5640_TIMING_DVPHO_LOW, 0xA0},
      {OV5640_TIMING_DVPVO_HIGH, 0x00},
      {OV5640_TIMING_DVPVO_LOW, 0x78},
  };

  /* 检查分辨率是否支持 */
  if (Resolution > OV5640_R400x300) {
    ret = OV5640_ERROR;
  } else {
    /* 初始化 OV5640 */
    switch (Resolution) {
    case OV5640_R400x300:
      for (index = 0; index < (sizeof(OV5640_400x300) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_400x300[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_400x300[index][0], &tmp, 1) !=
              OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
      break;
    case OV5640_R160x120:
      for (index = 0; index < (sizeof(OV5640_QQVGA) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_QQVGA[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_QQVGA[index][0], &tmp, 1) !=
              OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
      break;
    case OV5640_R320x240:
      for (index = 0; index < (sizeof(OV5640_QVGA) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_QVGA[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_QVGA[index][0], &tmp, 1) !=
              OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
      break;
    case OV5640_R480x272:
      for (index = 0; index < (sizeof(OV5640_480x272) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_480x272[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_480x272[index][0], &tmp, 1) !=
              OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
      break;
    case OV5640_R640x480:
      for (index = 0; index < (sizeof(OV5640_VGA) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_VGA[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_VGA[index][0], &tmp, 1) !=
              OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
      break;
    case OV5640_R800x480:
      for (index = 0; index < (sizeof(OV5640_WVGA) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_WVGA[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_WVGA[index][0], &tmp, 1) !=
              OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
      break;
    default:
      ret = OV5640_ERROR;
      break;
    }
  }

  return ret;
}

/**
 * @brief  获取 OV5640 摄像头分辨率。
 * @param  pObj  组件对象指针
 * @param  Resolution  摄像头分辨率指针
 * @retval 组件状态
 */
int32_t OV5640_GetResolution(OV5640_Object_t *pObj, uint32_t *Resolution) {
  int32_t ret;
  uint16_t x_size;
  uint16_t y_size;
  uint8_t tmp;

  if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_DVPHO_HIGH, &tmp, 1) !=
      OV5640_OK) {
    ret = OV5640_ERROR;
  } else {
    x_size = (uint16_t)tmp << 8U;

    if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_DVPHO_LOW, &tmp, 1) !=
        OV5640_OK) {
      ret = OV5640_ERROR;
    } else {
      x_size |= tmp;

      if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_DVPVO_HIGH, &tmp, 1) !=
          OV5640_OK) {
        ret = OV5640_ERROR;
      } else {
        y_size = (uint16_t)tmp << 8U;
        if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_DVPVO_LOW, &tmp, 1) !=
            OV5640_OK) {
          ret = OV5640_ERROR;
        } else {
          y_size |= tmp;

          if ((x_size == 800U) && (y_size == 480U)) {
            *Resolution = OV5640_R800x480;
            ret = OV5640_OK;
          } else if ((x_size == 400U) && (y_size == 300U)) {
            *Resolution = OV5640_R400x300;
            ret = OV5640_OK;
          } else if ((x_size == 640U) && (y_size == 480U)) {
            *Resolution = OV5640_R640x480;
            ret = OV5640_OK;
          } else if ((x_size == 480U) && (y_size == 272U)) {
            *Resolution = OV5640_R480x272;
            ret = OV5640_OK;
          } else if ((x_size == 320U) && (y_size == 240U)) {
            *Resolution = OV5640_R320x240;
            ret = OV5640_OK;
          } else if ((x_size == 160U) && (y_size == 120U)) {
            *Resolution = OV5640_R160x120;
            ret = OV5640_OK;
          } else {
            ret = OV5640_ERROR;
          }
        }
      }
    }
  }

  return ret;
}

/**
 * @brief  设置 OV5640 摄像头 PCLK、HREF 和 VSYNC 极性。
 * @param  pObj  组件对象指针
 * @param  PclkPolarity 像素时钟极性
 * @param  HrefPolarity Href 极性
 * @param  VsyncPolarity Vsync 极性
 * @retval 组件状态
 */
int32_t OV5640_SetPolarities(OV5640_Object_t *pObj, uint32_t PclkPolarity,
                             uint32_t HrefPolarity, uint32_t VsyncPolarity) {
  uint8_t tmp;
  int32_t ret = OV5640_OK;

  if ((pObj == NULL) ||
      ((PclkPolarity != OV5640_POLARITY_PCLK_LOW) &&
       (PclkPolarity != OV5640_POLARITY_PCLK_HIGH)) ||
      ((HrefPolarity != OV5640_POLARITY_HREF_LOW) &&
       (HrefPolarity != OV5640_POLARITY_HREF_HIGH)) ||
      ((VsyncPolarity != OV5640_POLARITY_VSYNC_LOW) &&
       (VsyncPolarity != OV5640_POLARITY_VSYNC_HIGH))) {
    ret = OV5640_ERROR;
  } else {
    tmp = (uint8_t)(PclkPolarity << 5U) | (HrefPolarity << 1U) | VsyncPolarity;

    if (ov5640_write_reg(&pObj->Ctx, OV5640_POLARITY_CTRL, &tmp, 1) !=
        OV5640_OK) {
      ret = OV5640_ERROR;
    }
  }

  return ret;
}

/**
 * @brief  获取 OV5640 摄像头 PCLK、HREF 和 VSYNC 极性。
 * @param  pObj  组件对象指针
 * @param  PclkPolarity 像素时钟极性指针
 * @param  HrefPolarity Href 极性指针
 * @param  VsyncPolarity Vsync 极性指针
 * @retval 组件状态
 */
int32_t OV5640_GetPolarities(OV5640_Object_t *pObj, uint32_t *PclkPolarity,
                             uint32_t *HrefPolarity, uint32_t *VsyncPolarity) {
  uint8_t tmp;
  int32_t ret = OV5640_OK;

  if ((pObj == NULL) || (PclkPolarity == NULL) || (HrefPolarity == NULL) ||
      (VsyncPolarity == NULL)) {
    ret = OV5640_ERROR;
  } else if (ov5640_read_reg(&pObj->Ctx, OV5640_POLARITY_CTRL, &tmp, 1) !=
             OV5640_OK) {
    ret = OV5640_ERROR;
  } else {
    *PclkPolarity = (tmp >> 5U) & 0x01U;
    *HrefPolarity = (tmp >> 1U) & 0x01U;
    *VsyncPolarity = tmp & 0x01;
  }

  return ret;
}

/**
 * @brief  读取 OV5640 摄像头标识。
 * @param  pObj  组件对象指针
 * @param  Id    组件 ID 指针
 * @retval 组件状态
 */
int32_t OV5640_ReadID(OV5640_Object_t *pObj, uint32_t *Id) {
  int32_t ret;
  uint8_t tmp;

  /* 初始化 I2C */
  pObj->IO.Init();

  /* 准备配置摄像头 */
  tmp = 0x80;
  if (ov5640_write_reg(&pObj->Ctx, OV5640_SYSTEM_CTROL0, &tmp, 1) !=
      OV5640_OK) {
    ret = OV5640_ERROR;
  } else {
    (void)OV5640_Delay(pObj, 500);

    if (ov5640_read_reg(&pObj->Ctx, OV5640_CHIP_ID_HIGH_BYTE, &tmp, 1) !=
        OV5640_OK) {
      ret = OV5640_ERROR;
    } else {
      *Id = (uint32_t)tmp << 8U;
      if (ov5640_read_reg(&pObj->Ctx, OV5640_CHIP_ID_LOW_BYTE, &tmp, 1) !=
          OV5640_OK) {
        ret = OV5640_ERROR;
      } else {
        *Id |= tmp;
        ret = OV5640_OK;
      }
    }
  }

  /* 组件状态 */
  return ret;
}

/**
 * @brief  读取 OV5640 摄像头能力。
 * @param  pObj  组件对象指针
 * @param  Capabilities  组件能力指针
 * @retval 组件状态
 */
int32_t OV5640_GetCapabilities(OV5640_Object_t *pObj,
                               OV5640_Capabilities_t *Capabilities) {
  int32_t ret;

  if (pObj == NULL) {
    ret = OV5640_ERROR;
  } else {
    Capabilities->Config_Brightness = 1;
    Capabilities->Config_Contrast = 1;
    Capabilities->Config_HueDegree = 1;
    Capabilities->Config_LightMode = 1;
    Capabilities->Config_MirrorFlip = 1;
    Capabilities->Config_NightMode = 1;
    Capabilities->Config_Resolution = 1;
    Capabilities->Config_Saturation = 1;
    Capabilities->Config_SpecialEffect = 1;
    Capabilities->Config_Zoom = 1;

    ret = OV5640_OK;
  }

  return ret;
}

/**
 * @brief  设置 OV5640 摄像头光照模式。
 * @param  pObj  组件对象指针
 * @param  LightMode 待配置的光照模式
 * @retval 组件状态
 */
int32_t OV5640_SetLightMode(OV5640_Object_t *pObj, uint32_t LightMode) {
  int32_t ret;
  uint32_t index;
  uint8_t tmp;

  /* OV5640 光照模式设置 */
  static const uint16_t OV5640_LightModeAuto[][2] = {
      {OV5640_AWB_MANUAL_CONTROL, 0x00}, {OV5640_AWB_R_GAIN_MSB, 0x04},
      {OV5640_AWB_R_GAIN_LSB, 0x00},     {OV5640_AWB_G_GAIN_MSB, 0x04},
      {OV5640_AWB_G_GAIN_LSB, 0x00},     {OV5640_AWB_B_GAIN_MSB, 0x04},
      {OV5640_AWB_B_GAIN_LSB, 0x00},
  };

  static const uint16_t OV5640_LightModeCloudy[][2] = {
      {OV5640_AWB_MANUAL_CONTROL, 0x01}, {OV5640_AWB_R_GAIN_MSB, 0x06},
      {OV5640_AWB_R_GAIN_LSB, 0x48},     {OV5640_AWB_G_GAIN_MSB, 0x04},
      {OV5640_AWB_G_GAIN_LSB, 0x00},     {OV5640_AWB_B_GAIN_MSB, 0x04},
      {OV5640_AWB_B_GAIN_LSB, 0xD3},
  };

  static const uint16_t OV5640_LightModeOffice[][2] = {
      {OV5640_AWB_MANUAL_CONTROL, 0x01}, {OV5640_AWB_R_GAIN_MSB, 0x05},
      {OV5640_AWB_R_GAIN_LSB, 0x48},     {OV5640_AWB_G_GAIN_MSB, 0x04},
      {OV5640_AWB_G_GAIN_LSB, 0x00},     {OV5640_AWB_B_GAIN_MSB, 0x07},
      {OV5640_AWB_B_GAIN_LSB, 0xCF},
  };

  static const uint16_t OV5640_LightModeHome[][2] = {
      {OV5640_AWB_MANUAL_CONTROL, 0x01}, {OV5640_AWB_R_GAIN_MSB, 0x04},
      {OV5640_AWB_R_GAIN_LSB, 0x10},     {OV5640_AWB_G_GAIN_MSB, 0x04},
      {OV5640_AWB_G_GAIN_LSB, 0x00},     {OV5640_AWB_B_GAIN_MSB, 0x08},
      {OV5640_AWB_B_GAIN_LSB, 0xB6},
  };

  static const uint16_t OV5640_LightModeSunny[][2] = {
      {OV5640_AWB_MANUAL_CONTROL, 0x01}, {OV5640_AWB_R_GAIN_MSB, 0x06},
      {OV5640_AWB_R_GAIN_LSB, 0x1C},     {OV5640_AWB_G_GAIN_MSB, 0x04},
      {OV5640_AWB_G_GAIN_LSB, 0x00},     {OV5640_AWB_B_GAIN_MSB, 0x04},
      {OV5640_AWB_B_GAIN_LSB, 0xF3},
  };

  tmp = 0x00;
  ret = ov5640_write_reg(&pObj->Ctx, OV5640_AWB_MANUAL_CONTROL, &tmp, 1);
  if (ret == OV5640_OK) {
    tmp = 0x46;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_AWB_CTRL16, &tmp, 1);
  }

  if (ret == OV5640_OK) {
    tmp = 0xF8;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_AWB_CTRL17, &tmp, 1);
  }

  if (ret == OV5640_OK) {
    tmp = 0x04;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_AWB_CTRL18, &tmp, 1);
  }

  if (ret == OV5640_OK) {
    switch (LightMode) {
    case OV5640_LIGHT_SUNNY:
      for (index = 0; index < (sizeof(OV5640_LightModeSunny) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_LightModeSunny[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_LightModeSunny[index][0],
                               &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
      break;
    case OV5640_LIGHT_OFFICE:
      for (index = 0; index < (sizeof(OV5640_LightModeOffice) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_LightModeOffice[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_LightModeOffice[index][0],
                               &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
      break;
    case OV5640_LIGHT_CLOUDY:
      for (index = 0; index < (sizeof(OV5640_LightModeCloudy) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_LightModeCloudy[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_LightModeCloudy[index][0],
                               &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
      break;
    case OV5640_LIGHT_HOME:
      for (index = 0; index < (sizeof(OV5640_LightModeHome) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_LightModeHome[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_LightModeHome[index][0], &tmp,
                               1) != OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
      break;
    case OV5640_LIGHT_AUTO:
    default:
      for (index = 0; index < (sizeof(OV5640_LightModeAuto) / 4U); index++) {
        if (ret != OV5640_ERROR) {
          tmp = (uint8_t)OV5640_LightModeAuto[index][1];
          if (ov5640_write_reg(&pObj->Ctx, OV5640_LightModeAuto[index][0], &tmp,
                               1) != OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
      }
      break;
    }
  }
  return ret;
}

/**
 * @brief  设置 OV5640 摄像头特效。
 * @param  pObj  组件对象指针
 * @param  Effect 待配置的特效
 * @retval 组件状态
 */
int32_t OV5640_SetColorEffect(OV5640_Object_t *pObj, uint32_t Effect) {
  int32_t ret;
  uint8_t tmp;

  switch (Effect) {
  case OV5640_COLOR_EFFECT_BLUE:
    tmp = 0xFF;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

    if (ret == OV5640_OK) {
      tmp = 0x18;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0xA0;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x40;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
    }

    if (ret != OV5640_OK) {
      ret = OV5640_ERROR;
    }
    break;

  case OV5640_COLOR_EFFECT_RED:
    tmp = 0xFF;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

    if (ret == OV5640_OK) {
      tmp = 0x18;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x80;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0xC0;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
    }

    if (ret != OV5640_OK) {
      ret = OV5640_ERROR;
    }
    break;

  case OV5640_COLOR_EFFECT_GREEN:
    tmp = 0xFF;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

    if (ret == OV5640_OK) {
      tmp = 0x18;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x60;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x60;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
    }

    if (ret != OV5640_OK) {
      ret = OV5640_ERROR;
    }
    break;

  case OV5640_COLOR_EFFECT_BW:
    tmp = 0xFF;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

    if (ret == OV5640_OK) {
      tmp = 0x18;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x80;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x80;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
    }

    if (ret != OV5640_OK) {
      ret = OV5640_ERROR;
    }
    break;

  case OV5640_COLOR_EFFECT_SEPIA:
    tmp = 0xFF;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

    if (ret == OV5640_OK) {
      tmp = 0x18;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x40;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0xA0;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
    }

    if (ret != OV5640_OK) {
      ret = OV5640_ERROR;
    }
    break;

  case OV5640_COLOR_EFFECT_NEGATIVE:
    tmp = 0xFF;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

    if (ret == OV5640_OK) {
      tmp = 0x40;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
    }
    if (ret != OV5640_OK) {
      ret = OV5640_ERROR;
    }
    break;

  case OV5640_COLOR_EFFECT_NONE:
  default:
    tmp = 0x7F;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

    if (ret == OV5640_OK) {
      tmp = 0x00;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
    }

    if (ret != OV5640_OK) {
      ret = OV5640_ERROR;
    }

    break;
  }

  return ret;
}

/**
 * @brief  设置 OV5640 摄像头亮度等级。
 * @note   OV5640 亮度可调。较高亮度会使画面更亮，副作用是画面会显得发雾。
 * @param  pObj  组件对象指针
 * @param  Level 待配置的等级
 * @retval 组件状态
 */
int32_t OV5640_SetBrightness(OV5640_Object_t *pObj, int32_t Level) {
  int32_t ret;
  const uint8_t brightness_level[] = {0x40U, 0x30U, 0x20U, 0x10U, 0x00U,
                                      0x10U, 0x20U, 0x30U, 0x40U};
  uint8_t tmp;

  tmp = 0xFF;
  ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

  if (ret == OV5640_OK) {
    tmp = brightness_level[Level + 4];
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL7, &tmp, 1);
  }
  if (ret == OV5640_OK) {
    tmp = 0x04;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
  }

  if (ret == OV5640_OK) {
    if (Level < 0) {
      tmp = 0x01;
      if (ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL8, &tmp, 1) !=
          OV5640_OK) {
        ret = OV5640_ERROR;
      }
    } else {
      tmp = 0x09;
      if (ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL8, &tmp, 1) !=
          OV5640_OK) {
        ret = OV5640_ERROR;
      }
    }
  }

  return ret;
}

/**
 * @brief  设置 OV5640 摄像头饱和度等级。
 * @note   OV5640
 * 色彩饱和度可调。高饱和度会使画面更鲜艳，但副作用是噪点更大、肤色不准。
 * @param  pObj  组件对象指针
 * @param  Level 待配置的等级
 * @retval 组件状态
 */
int32_t OV5640_SetSaturation(OV5640_Object_t *pObj, int32_t Level) {
  int32_t ret;
  const uint8_t saturation_level[] = {0x00U, 0x10U, 0x20U, 0x30U, 0x80U,
                                      0x70U, 0x60U, 0x50U, 0x40U};
  uint8_t tmp;

  tmp = 0xFF;
  ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

  if (ret == OV5640_OK) {
    tmp = saturation_level[Level + 4];
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
  }
  if (ret == OV5640_OK) {
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
  }
  if (ret == OV5640_OK) {
    tmp = 0x02;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
  }

  if (ret == OV5640_OK) {
    tmp = 0x41;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL8, &tmp, 1);
  }

  if (ret != OV5640_OK) {
    ret = OV5640_ERROR;
  }

  return ret;
}

/**
 * @brief  设置 OV5640 摄像头对比度等级。
 * @note   OV5640 对比度可调。较高对比度会使画面更锐利，但会损失动态范围。
 * @param  pObj  组件对象指针
 * @param  Level 待配置的等级
 * @retval 组件状态
 */
int32_t OV5640_SetContrast(OV5640_Object_t *pObj, int32_t Level) {
  int32_t ret;
  const uint8_t contrast_level[] = {0x10U, 0x14U, 0x18U, 0x1CU, 0x20U,
                                    0x24U, 0x28U, 0x2CU, 0x30U};
  uint8_t tmp;

  tmp = 0xFF;
  ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

  if (ret == OV5640_OK) {
    tmp = 0x04;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
  }
  if (ret == OV5640_OK) {
    tmp = contrast_level[Level + 4];
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL6, &tmp, 1);
  }
  if (ret == OV5640_OK) {
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL5, &tmp, 1);
  }
  if (ret == OV5640_OK) {
    tmp = 0x41;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL8, &tmp, 1);
  }

  if (ret != OV5640_OK) {
    ret = OV5640_ERROR;
  }

  return ret;
}

/**
 * @brief  设置 OV5640 摄像头色相角度。
 * @param  pObj  组件对象指针
 * @param  Degree 待配置的角度
 * @retval 组件状态
 */
int32_t OV5640_SetHueDegree(OV5640_Object_t *pObj, int32_t Degree) {
  int32_t ret;
  const uint8_t hue_degree_ctrl1[] = {0x80U, 0x6FU, 0x40U, 0x00U, 0x40U, 0x6FU,
                                      0x80U, 0x6FU, 0x40U, 0x00U, 0x40U, 0x6FU};
  const uint8_t hue_degree_ctrl2[] = {0x00U, 0x40U, 0x6FU, 0x80U, 0x6FU, 0x40U,
                                      0x00U, 0x40U, 0x6FU, 0x80U, 0x6FU, 0x40U};
  const uint8_t hue_degree_ctrl8[] = {0x32U, 0x32U, 0x32U, 0x02U, 0x02U, 0x02U,
                                      0x01U, 0x01U, 0x01U, 0x31U, 0x31U, 0x31U};
  uint8_t tmp;

  tmp = 0xFF;
  ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

  if (ret == OV5640_OK) {
    tmp = 0x01;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
  }
  if (ret == OV5640_OK) {
    tmp = hue_degree_ctrl1[Degree + 6];
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL1, &tmp, 1);
  }
  if (ret == OV5640_OK) {
    tmp = hue_degree_ctrl2[Degree + 6];
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL2, &tmp, 1);
  }
  if (ret == OV5640_OK) {
    tmp = hue_degree_ctrl8[Degree + 6];
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL8, &tmp, 1);
  }

  if (ret != OV5640_OK) {
    ret = OV5640_ERROR;
  }

  return ret;
}

/**
 * @brief  控制 OV5640 摄像头镜像/翻转。
 * @param  pObj  组件对象指针
 * @param  Config 配置镜像、翻转、两者或无
 * @retval 组件状态
 */
int32_t OV5640_MirrorFlipConfig(OV5640_Object_t *pObj, uint32_t Config) {
  int32_t ret;
  uint8_t tmp3820 = 0;
  uint8_t tmp3821;

  if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_TC_REG20, &tmp3820, 1) !=
      OV5640_OK) {
    ret = OV5640_ERROR;
  } else {
    tmp3820 &= 0xF9U;

    if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp3821, 1) !=
        OV5640_OK) {
      ret = OV5640_ERROR;
    } else {
      ret = OV5640_OK;
      tmp3821 &= 0xF9U;

      switch (Config) {
      case OV5640_MIRROR:
        if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG20, &tmp3820, 1) !=
            OV5640_OK) {
          ret = OV5640_ERROR;
        } else {
          tmp3821 |= 0x06U;
          if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp3821,
                               1) != OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
        break;
      case OV5640_FLIP:
        tmp3820 |= 0x06U;
        if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG20, &tmp3820, 1) !=
            OV5640_OK) {
          ret = OV5640_ERROR;
        } else {
          if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp3821,
                               1) != OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
        break;
      case OV5640_MIRROR_FLIP:
        tmp3820 |= 0x06U;
        if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG20, &tmp3820, 1) !=
            OV5640_OK) {
          ret = OV5640_ERROR;
        } else {
          tmp3821 |= 0x06U;
          if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp3821,
                               1) != OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
        break;

      case OV5640_MIRROR_FLIP_NONE:
      default:
        if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG20, &tmp3820, 1) !=
            OV5640_OK) {
          ret = OV5640_ERROR;
        } else {
          if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp3821,
                               1) != OV5640_OK) {
            ret = OV5640_ERROR;
          }
        }
        break;
      }
    }
  }

  return ret;
}

/**
 * @brief  控制 OV5640 摄像头变焦。
 * @param  pObj  组件对象指针
 * @param  Zoom 待配置的变焦
 * @retval 组件状态
 */
int32_t OV5640_ZoomConfig(OV5640_Object_t *pObj, uint32_t Zoom) {
  int32_t ret = OV5640_OK;
  uint32_t res;
  uint32_t zoom;
  uint8_t tmp;

  /* 获取摄像头分辨率 */
  if (OV5640_GetResolution(pObj, &res) != OV5640_OK) {
    ret = OV5640_ERROR;
  } else {
    zoom = Zoom;

    if (zoom == OV5640_ZOOM_x1) {
      tmp = 0x10;
      if (ov5640_write_reg(&pObj->Ctx, OV5640_SCALE_CTRL0, &tmp, 1) !=
          OV5640_OK) {
        ret = OV5640_ERROR;
      }
    } else {
      switch (res) {
      case OV5640_R320x240:
      case OV5640_R480x272:
        zoom = zoom >> 1U;
        break;
      case OV5640_R640x480:
        zoom = zoom >> 2U;
        break;
      default:
        break;
      }

      tmp = 0x00;
      if (ov5640_write_reg(&pObj->Ctx, OV5640_SCALE_CTRL0, &tmp, 1) !=
          OV5640_OK) {
        ret = OV5640_ERROR;
      } else {
        tmp = (uint8_t)zoom;
        if (ov5640_write_reg(&pObj->Ctx, OV5640_SCALE_CTRL1, &tmp, 1) !=
            OV5640_OK) {
          ret = OV5640_ERROR;
        }
      }
    }
  }

  return ret;
}

/**
 * @brief  启用/禁用 OV5640 摄像头夜景模式。
 * @param  pObj  组件对象指针
 * @param  Cmd 启用或禁用夜景模式
 * @retval 组件状态
 */
int32_t OV5640_NightModeConfig(OV5640_Object_t *pObj, uint32_t Cmd) {
  int32_t ret;
  uint8_t tmp = 0;

  if (Cmd == NIGHT_MODE_ENABLE) {
    /* 自动帧率: 60/50Hz 光源环境下 15fps~3.75fps 夜景模式，
    24MHz 时钟输入，24MHz PCLK */
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL4, &tmp, 1);
    if (ret == OV5640_OK) {
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL5, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x7C;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL00, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x01;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_B50_STEP_HIGH, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x27;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_B50_STEP_LOW, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x00;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_B60_STEP_HIGH, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0xF6;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_B60_STEP_LOW, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x04;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL0D, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL0E, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x0B;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL02, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x88;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL03, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x0B;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_MAX_EXPO_HIGH, &tmp, 1);
    }
    if (ret == OV5640_OK) {
      tmp = 0x88;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_MAX_EXPO_LOW, &tmp, 1);
    }
    if (ret != OV5640_OK) {
      ret = OV5640_ERROR;
    }
  } else {
    if (ov5640_read_reg(&pObj->Ctx, OV5640_AEC_CTRL00, &tmp, 1) != OV5640_OK) {
      ret = OV5640_ERROR;
    } else {
      ret = OV5640_OK;
      tmp &= 0xFBU;
      /* 将 bit 2 置 0 */
      if (ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL00, &tmp, 1) !=
          OV5640_OK) {
        ret = OV5640_ERROR;
      }
    }
  }

  return ret;
}
/**
 * @brief  配置嵌入式同步模式。
 * @param  pObj  组件对象指针
 * @param  pSyncCodes  嵌入式同步码指针
 * @retval 组件状态
 */

int32_t OV5640_EmbeddedSynchroConfig(OV5640_Object_t *pObj,
                                     OV5640_SyncCodes_t *pSyncCodes) {
  uint8_t tmp;
  int32_t ret = OV5640_ERROR;

  /*[7]: 同步码来自 reg 0x4732-0x4732, [1]: 使能裁剪, [0]: 使能 CCIR656 */
  tmp = 0x83;
  if (ov5640_write_reg(&pObj->Ctx, OV5640_CCIR656_CTRL00, &tmp, 1) ==
      OV5640_OK) {
    tmp = pSyncCodes->FrameStartCode;
    if (ov5640_write_reg(&pObj->Ctx, OV5640_CCIR656_FS, &tmp, 1) == OV5640_OK) {
      tmp = pSyncCodes->FrameEndCode;
      if (ov5640_write_reg(&pObj->Ctx, OV5640_CCIR656_FE, &tmp, 1) !=
          OV5640_OK) {
        return OV5640_ERROR;
      }
      tmp = pSyncCodes->LineStartCode;
      if (ov5640_write_reg(&pObj->Ctx, OV5640_CCIR656_LS, &tmp, 1) ==
          OV5640_OK) {
        tmp = pSyncCodes->LineEndCode;
        if (ov5640_write_reg(&pObj->Ctx, OV5640_CCIR656_LE, &tmp, 1) ==
            OV5640_OK) {
          /* 添加 1 条哑线 */
          tmp = 0x01;
          if (ov5640_write_reg(&pObj->Ctx, OV5640_656_DUMMY_LINE, &tmp, 1) ==
              OV5640_OK) {
            ret = OV5640_OK;
          }
        }
      }
    }
  }

  /* 最大裁剪值[9:8]，避免同步码被裁剪 */
  tmp = 0x2;
  if (ret == OV5640_OK) {
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_YMAX_VAL_HIGH, &tmp, 1);
  }
  if (ret == OV5640_OK) {
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_UMAX_VAL_HIGH, &tmp, 1);
  }
  if (ret == OV5640_OK) {
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_VMAX_VAL_HIGH, &tmp, 1);
  }

  return ret;
}
/**
 * @brief  启用/禁用 OV5640 彩条模式。
 * @param  pObj  组件对象指针
 * @param  Cmd 启用或禁用彩条
 * @retval 组件状态
 */
int32_t OV5640_ColorbarModeConfig(OV5640_Object_t *pObj, uint32_t Cmd) {
  int32_t ret;
  uint8_t tmp = 0x40;

  if ((Cmd == COLORBAR_MODE_ENABLE) || (Cmd == COLORBAR_MODE_GRADUALV)) {
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
    if (ret == OV5640_OK) {
      tmp = (Cmd == COLORBAR_MODE_GRADUALV ? 0x8c : 0x80);
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_PRE_ISP_TEST_SETTING1, &tmp, 1);
    }
    if (ret != OV5640_OK) {
      ret = OV5640_ERROR;
    }
  } else {
    tmp = 0x10;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
    if (ret == OV5640_OK) {
      tmp = 0x00;
      ret = ov5640_write_reg(&pObj->Ctx, OV5640_PRE_ISP_TEST_SETTING1, &tmp, 1);
    }
    if (ret != OV5640_OK) {
      ret = OV5640_ERROR;
    }
  }

  return ret;
}

/**
 * @brief  设置摄像头像素时钟。
 * @param  pObj  组件对象指针
 * @param  ClockValue 可为
 * OV5640_PCLK_48M、OV5640_PCLK_24M、OV5640_PCLK_12M、OV5640_PCLK_9M、
 *                    OV5640_PCLK_8M、OV5640_PCLK_7M
 * @retval 组件状态
 */
int32_t OV5640_SetPCLK(OV5640_Object_t *pObj, uint32_t ClockValue) {
  int32_t ret;
  uint8_t tmp;

  switch (ClockValue) {
  case OV5640_PCLK_7M:
    tmp = 0x38;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
    tmp = 0x16;
    ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
    break;
  case OV5640_PCLK_8M:
    tmp = 0x40;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
    tmp = 0x16;
    ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
    break;
  case OV5640_PCLK_9M:
    tmp = 0x60;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
    tmp = 0x18;
    ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
    break;
  case OV5640_PCLK_12M:
    tmp = 0x60;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
    tmp = 0x16;
    ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
    break;
  case OV5640_PCLK_48M:
    tmp = 0x60;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
    tmp = 0x03;
    ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
    break;
  case OV5640_PCLK_24M:
  default:
    tmp = 0x60;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
    tmp = 0x13;
    ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
    break;
  }

  if (ret != OV5640_OK) {
    ret = OV5640_ERROR;
  }

  return ret;
}

/**
 * @brief  启用 DVP（数字视频端口）模式：并行数据输出。
 * @param  pObj  组件对象指针
 * @retval 组件状态
 */
int OV5640_EnableDVPMode(OV5640_Object_t *pObj) {
  uint32_t index;
  int32_t ret = OV5640_OK;
  uint8_t tmp;

  static const uint16_t regs[10][2] = {
      /* 配置 IO 焊盘，输出 FREX/VSYNC/HREF/PCLK/D[9:2]/GPIO0/GPIO1 */
      {OV5640_PAD_OUTPUT_ENABLE01, 0xFF},
      {OV5640_PAD_OUTPUT_ENABLE02, 0xF3},
      {0x302e, 0x00},
      /* 未知 DVP 控制配置 */
      {0x471c, 0x50},
      {OV5640_MIPI_CONTROL00, 0x58},
      /* 时序配置 */
      {OV5640_SC_PLL_CONTRL0, 0x18},
      {OV5640_SC_PLL_CONTRL1, 0x41},
      {OV5640_SC_PLL_CONTRL2, 0x60},
      {OV5640_SC_PLL_CONTRL3, 0x13},
      {OV5640_SYSTEM_ROOT_DIVIDER, 0x01},
  };

  for (index = 0; index < sizeof(regs) / 4U; index++) {
    tmp = (uint8_t)regs[index][1];
    if (ov5640_write_reg(&pObj->Ctx, regs[index][0], &tmp, 1) != OV5640_OK) {
      ret = OV5640_ERROR;
      break;
    }
  }

  return ret;
}

/**
 * @brief  启用 MIPI（移动产业处理器接口）模式：串口。
 * @param  pObj  组件对象指针
 * @retval 组件状态
 */
int32_t OV5640_EnableMIPIMode(OV5640_Object_t *pObj) {
  int32_t ret = OV5640_OK;
  uint8_t tmp;
  uint32_t index;

  static const uint16_t regs[14][2] = {
      /* 焊盘设置 */
      {OV5640_PAD_OUTPUT_ENABLE01, 0},
      {OV5640_PAD_OUTPUT_ENABLE02, 0},
      {0x302e, 0x08},
      /* 像素时钟周期 */
      {OV5640_PCLK_PERIOD, 0x23},
      /* 时序配置 */
      {OV5640_SC_PLL_CONTRL0, 0x18},
      {OV5640_SC_PLL_CONTRL1, 0x12},
      {OV5640_SC_PLL_CONTRL2, 0x1C},
      {OV5640_SC_PLL_CONTRL3, 0x13},
      {OV5640_SYSTEM_ROOT_DIVIDER, 0x01},
      {0x4814, 0x2a},
      {OV5640_MIPI_CTRL00, 0x24},
      {OV5640_PAD_OUTPUT_VALUE00, 0x70},
      {OV5640_MIPI_CONTROL00, 0x45},
      {OV5640_FRAME_CTRL02, 0x00},
  };

  for (index = 0; index < sizeof(regs) / 4U; index++) {
    tmp = (uint8_t)regs[index][1];
    if (ov5640_write_reg(&pObj->Ctx, regs[index][0], &tmp, 1) != OV5640_OK) {
      ret = OV5640_ERROR;
      break;
    }
  }

  return ret;
}

/**
 * @brief  设置 MIPI 虚拟通道。
 * @param  pObj  组件对象指针
 * @param  vchannel MIPI 模式的虚拟通道
 * @retval 组件状态
 */
int32_t OV5640_SetMIPIVirtualChannel(OV5640_Object_t *pObj, uint32_t vchannel) {
  int32_t ret = OV5640_OK;
  uint8_t tmp;

  if (ov5640_read_reg(&pObj->Ctx, 0x4814, &tmp, 1) != OV5640_OK) {
    ret = OV5640_ERROR;
  } else {
    tmp &= ~(3 << 6);
    tmp |= (vchannel << 6);
    if (ov5640_write_reg(&pObj->Ctx, 0x4814, &tmp, 1) != OV5640_OK) {
      ret = OV5640_ERROR;
    }
  }

  return ret;
}

/**
 * @brief  启动摄像头。
 * @param  pObj  组件对象指针
 * @retval 组件状态
 */
int32_t OV5640_Start(OV5640_Object_t *pObj) {
  uint8_t tmp;

  tmp = 0x2;
  return ov5640_write_reg(&pObj->Ctx, OV5640_SYSTEM_CTROL0, &tmp, 1);
}

/**
 * @brief  停止摄像头。
 * @param  pObj  组件对象指针
 * @retval 组件状态
 */
int32_t OV5640_Stop(OV5640_Object_t *pObj) {
  uint8_t tmp;

  tmp = 0x42;
  return ov5640_write_reg(&pObj->Ctx, OV5640_SYSTEM_CTROL0, &tmp, 1);
}

/**
 * @}
 */

/** @defgroup OV5640_Private_Functions 私有函数
 * @{
 */
/**
 * @brief 本函数提供精确延时（单位：毫秒）。
 * @param pObj  组件对象指针
 * @param Delay 指定延时时间长度，单位毫秒
 * @retval OV5640_OK
 */
static int32_t OV5640_Delay(OV5640_Object_t *pObj, uint32_t Delay) {
  uint32_t tickstart;
  tickstart = pObj->IO.GetTick();
  while ((pObj->IO.GetTick() - tickstart) < Delay) {
  }
  return OV5640_OK;
}

/**
 * @brief  将组件 ReadReg 封装为总线读函数。
 * @param  handle  组件对象句柄
 * @param  Reg  要读取的目标寄存器地址
 * @param  pData  要读取的数据缓冲区
 * @param  Length  要读取的数据长度
 * @retval 错误状态
 */
static int32_t OV5640_ReadRegWrap(void *handle, uint16_t Reg, uint8_t *pData,
                                  uint16_t Length) {
  OV5640_Object_t *pObj = (OV5640_Object_t *)handle;

  return pObj->IO.ReadReg(pObj->IO.Address, Reg, pData, Length);
}

/**
 * @brief  将组件 WriteReg 封装为总线写函数。
 * @param  handle  组件对象句柄
 * @param  Reg  要写入的目标寄存器地址
 * @param  pData  要写入的寄存器值
 * @param  Length  要写入的数据长度
 * @retval 错误状态
 */
static int32_t OV5640_WriteRegWrap(void *handle, uint16_t Reg, uint8_t *pData,
                                   uint16_t Length) {
  OV5640_Object_t *pObj = (OV5640_Object_t *)handle;

  return pObj->IO.WriteReg(pObj->IO.Address, Reg, pData, Length);
}
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
