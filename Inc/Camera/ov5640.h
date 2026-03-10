/**
  ******************************************************************************
  * @file    ov5640.h
  * @author  MCD Application Team
  * @brief   本文件包含 ov5640.c 驱动的所有函数原型。
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

/* 防止重复包含 -------------------------------------*/
#ifndef OV5640_H
#define OV5640_H

#ifdef __cplusplus
extern "C" {
#endif

/* 头文件包含 ------------------------------------------------------------------*/
#include "ov5640_reg.h"
#include <stddef.h>

/** @addtogroup BSP
  * @{
  */

/** @addtogroup Components
  * @{
  */

/** @addtogroup ov5640
  * @{
  */

/** @defgroup OV5640_Exported_Types
  * @{
  */

typedef int32_t (*OV5640_Init_Func)(void);
typedef int32_t (*OV5640_DeInit_Func)(void);
typedef int32_t (*OV5640_GetTick_Func)(void);
typedef int32_t (*OV5640_Delay_Func)(uint32_t);
typedef int32_t (*OV5640_WriteReg_Func)(uint16_t, uint16_t, uint8_t *, uint16_t);
typedef int32_t (*OV5640_ReadReg_Func)(uint16_t, uint16_t, uint8_t *, uint16_t);

typedef struct
{
  OV5640_Init_Func          Init;
  OV5640_DeInit_Func        DeInit;
  uint16_t                  Address;
  OV5640_WriteReg_Func      WriteReg;
  OV5640_ReadReg_Func       ReadReg;
  OV5640_GetTick_Func       GetTick;
} OV5640_IO_t;


typedef struct
{
  OV5640_IO_t         IO;
  ov5640_ctx_t        Ctx;
  uint8_t             IsInitialized;
  uint8_t             Mode;
  uint32_t            VirtualChannelID;
} OV5640_Object_t;

typedef struct
{
  uint8_t FrameStartCode; /*!< 帧起始分隔符代码。 */
  uint8_t LineStartCode;  /*!< 行起始分隔符代码。  */
  uint8_t LineEndCode;    /*!< 行结束分隔符代码。    */
  uint8_t FrameEndCode;   /*!< 帧结束分隔符代码。   */

} OV5640_SyncCodes_t;

typedef struct
{
  uint32_t Config_Resolution;
  uint32_t Config_LightMode;
  uint32_t Config_SpecialEffect;
  uint32_t Config_Brightness;
  uint32_t Config_Saturation;
  uint32_t Config_Contrast;
  uint32_t Config_HueDegree;
  uint32_t Config_MirrorFlip;
  uint32_t Config_Zoom;
  uint32_t Config_NightMode;
} OV5640_Capabilities_t;

typedef struct
{
  int32_t (*Init)(OV5640_Object_t *, uint32_t, uint32_t);
  int32_t (*DeInit)(OV5640_Object_t *);
  int32_t (*ReadID)(OV5640_Object_t *, uint32_t *);
  int32_t (*GetCapabilities)(OV5640_Object_t *, OV5640_Capabilities_t *);
  int32_t (*SetLightMode)(OV5640_Object_t *, uint32_t);
  int32_t (*SetColorEffect)(OV5640_Object_t *, uint32_t);
  int32_t (*SetBrightness)(OV5640_Object_t *, int32_t);
  int32_t (*SetSaturation)(OV5640_Object_t *, int32_t);
  int32_t (*SetContrast)(OV5640_Object_t *, int32_t);
  int32_t (*SetHueDegree)(OV5640_Object_t *, int32_t);
  int32_t (*MirrorFlipConfig)(OV5640_Object_t *, uint32_t);
  int32_t (*ZoomConfig)(OV5640_Object_t *, uint32_t);
  int32_t (*SetResolution)(OV5640_Object_t *, uint32_t);
  int32_t (*GetResolution)(OV5640_Object_t *, uint32_t *);
  int32_t (*SetPixelFormat)(OV5640_Object_t *, uint32_t);
  int32_t (*GetPixelFormat)(OV5640_Object_t *, uint32_t *);
  int32_t (*NightModeConfig)(OV5640_Object_t *, uint32_t);
} OV5640_CAMERA_Drv_t;
/**
  * @}
  */

/** @defgroup OV5640_Exported_Constants
  * @{
  */
#define OV5640_OK                      (0)
#define OV5640_ERROR                   (-1)
/**
  * @brief  OV5640 特性参数
  */
/* 摄像头分辨率 */
#define OV5640_R160x120                 0x00U   /* QQVGA 分辨率           */
#define OV5640_R320x240                 0x01U   /* QVGA 分辨率            */
#define OV5640_R480x272                 0x02U   /* 480x272 分辨率         */
#define OV5640_R640x480                 0x03U   /* VGA 分辨率             */
#define OV5640_R800x480                 0x04U   /* WVGA 分辨率            */
#define OV5640_R400x300                 0x05U   /* 400x300（鹿小班参考例程 4:3） */

/* 摄像头像素格式 */
#define OV5640_RGB565                   0x00U   /* 像素格式 RGB565        */
#define OV5640_RGB888                   0x01U   /* 像素格式 RGB888        */
#define OV5640_YUV422                   0x02U   /* 像素格式 YUV422        */
#define OV5640_Y8                       0x07U   /* 像素格式 Y8            */
#define OV5640_JPEG                     0x08U   /* 压缩格式 JPEG          */

/* 极性 */
#define OV5640_POLARITY_PCLK_LOW        0x00U /* 信号低有效          */
#define OV5640_POLARITY_PCLK_HIGH       0x01U /* 信号高有效         */
#define OV5640_POLARITY_HREF_LOW        0x00U /* 信号低有效          */
#define OV5640_POLARITY_HREF_HIGH       0x01U /* 信号高有效         */
#define OV5640_POLARITY_VSYNC_LOW       0x01U /* 信号低有效          */
#define OV5640_POLARITY_VSYNC_HIGH      0x00U /* 信号高有效         */

/* 镜像/翻转 */
#define OV5640_MIRROR_FLIP_NONE         0x00U   /* 设置摄像头正常模式     */
#define OV5640_FLIP                     0x01U   /* 设置摄像头翻转配置     */
#define OV5640_MIRROR                   0x02U   /* 设置摄像头镜像配置   */
#define OV5640_MIRROR_FLIP              0x03U   /* 设置摄像头镜像并翻转 */

/* 变焦 */
#define OV5640_ZOOM_x8                  0x00U   /* 变焦 x8             */
#define OV5640_ZOOM_x4                  0x11U   /* 变焦 x4             */
#define OV5640_ZOOM_x2                  0x22U   /* 变焦 x2             */
#define OV5640_ZOOM_x1                  0x44U   /* 变焦 x1             */

/* 特效 */
#define OV5640_COLOR_EFFECT_NONE        0x00U   /* 无效果                  */
#define OV5640_COLOR_EFFECT_BLUE        0x01U   /* 蓝色效果                */
#define OV5640_COLOR_EFFECT_RED         0x02U   /* 红色效果                 */
#define OV5640_COLOR_EFFECT_GREEN       0x04U   /* 绿色效果               */
#define OV5640_COLOR_EFFECT_BW          0x08U   /* 黑白效果     */
#define OV5640_COLOR_EFFECT_SEPIA       0x10U   /* 棕褐色效果               */
#define OV5640_COLOR_EFFECT_NEGATIVE    0x20U   /* 负片效果            */


/* 光照模式 */
#define OV5640_LIGHT_AUTO               0x00U   /* 光照模式 自动            */
#define OV5640_LIGHT_SUNNY              0x01U   /* 光照模式 日光           */
#define OV5640_LIGHT_OFFICE             0x02U   /* 光照模式 办公室          */
#define OV5640_LIGHT_HOME               0x04U   /* 光照模式 家居            */
#define OV5640_LIGHT_CLOUDY             0x08U   /* 光照模式 阴天          */

/* 夜景模式 */
#define NIGHT_MODE_DISABLE              0x00U   /* 禁用夜景模式         */
#define NIGHT_MODE_ENABLE               0x01U   /* 启用夜景模式          */

/* 彩条模式 */
#define COLORBAR_MODE_DISABLE           0x00U   /* 禁用彩条模式      */
#define COLORBAR_MODE_ENABLE            0x01U   /* 8 条彩条 W/Y/C/G/M/R/B/Bl    */
#define COLORBAR_MODE_GRADUALV          0x02U   /* 垂直渐变彩条  */

/* 像素时钟 */
#define OV5640_PCLK_7M                  0x00U   /* 像素时钟设为 7MHz    */
#define OV5640_PCLK_8M                  0x01U   /* 像素时钟设为 8MHz    */
#define OV5640_PCLK_9M                  0x02U   /* 像素时钟设为 9MHz    */
#define OV5640_PCLK_12M                 0x04U   /* 像素时钟设为 12MHz   */
#define OV5640_PCLK_24M                 0x08U   /* 像素时钟设为 24MHz   */
#define OV5640_PCLK_48M                 0x09U   /* 像素时钟设为 48MHz   */

/* 模式 */
#define PARALLEL_MODE                   0x00U   /* 并行接口模式 */
#define SERIAL_MODE                     0x01U   /* 串行接口模式   */

/**
  * @}
  */

/** @defgroup OV5640_Exported_Functions OV5640 Exported Functions
  * @{
  */
int32_t OV5640_RegisterBusIO(OV5640_Object_t *pObj, OV5640_IO_t *pIO);
int32_t OV5640_SetPolarities(OV5640_Object_t *pObj, uint32_t PclkPolarity, uint32_t HrefPolarity,
                             uint32_t VsyncPolarity);
int32_t OV5640_GetPolarities(OV5640_Object_t *pObj, uint32_t *PclkPolarity, uint32_t *HrefPolarity,
                             uint32_t *VsyncPolarity);
int32_t OV5640_ColorbarModeConfig(OV5640_Object_t *pObj, uint32_t Cmd);
int32_t OV5640_EmbeddedSynchroConfig(OV5640_Object_t *pObj, OV5640_SyncCodes_t *pSyncCodes);
int32_t OV5640_SetPCLK(OV5640_Object_t *pObj, uint32_t ClockValue);

int OV5640_EnableDVPMode(OV5640_Object_t *pObj);
int32_t OV5640_EnableMIPIMode(OV5640_Object_t *pObj);
int32_t OV5640_SetMIPIVirtualChannel(OV5640_Object_t *pObj, uint32_t vchannel);
int32_t OV5640_Start(OV5640_Object_t *pObj);
int32_t OV5640_Stop(OV5640_Object_t *pObj);

/* CAMERA 驱动结构体，通过 OV5640_CAMERA_Driver.Init/ReadID/SetResolution 等调用 */
extern OV5640_CAMERA_Drv_t   OV5640_CAMERA_Driver;
/**
  * @}
  */
#ifdef __cplusplus
}
#endif

#endif /* OV5640_H */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
