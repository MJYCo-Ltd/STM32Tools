/**
 ******************************************************************************
 * @file    ov5640_user.h
 * @brief   OV5640 用户配置：I2C/SCCB 接口与 DCMI 集成
 ******************************************************************************
 * 使用步骤：
 * 1. CubeMX 中启用 I2C（如 I2C1），连接 OV5640 的 SIO_C(SCL)、SIO_D(SDA)
 * 2. 在工程中实现本头文件声明的接口，或使用 ov5640_bridge.c 模板
 * 3. 在初始化时调用 OV5640_RegisterBusIO、OV5640_Init
 * 4. 使用 HAL_DCMI_Start_DMA 启动图像采集
 ******************************************************************************
 */
#ifndef __OV5640_USER_H__
#define __OV5640_USER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"
#include "i2c.h"
#include <Camera/ov5640.h>

#define OV5640_I2C_HANDLE hi2c1
/// OV5640 I2C 地址（7 位，SCCB 兼容）
#define OV5640_I2C_ADDR (0x78)

static inline int32_t OV5640_USER_Init(){
    return OV5640_OK;
}

static inline int32_t OV5640_USER_DeInit(){
    return OV5640_OK;
}

static inline int32_t OV5640_USER_GetTick(){
    return HAL_GetTick();
}

static inline int32_t OV5640_USER_WriteReg(uint16_t devAddr, uint16_t Reg,
                                           uint8_t *pData, uint16_t Length) {
    if (HAL_I2C_Mem_Write(&hi2c1, devAddr, Reg, I2C_MEMADD_SIZE_16BIT, pData,
                         Length, HAL_MAX_DELAY) == HAL_OK) {
        return OV5640_OK;
    }

    return OV5640_ERROR;
}

static inline int32_t OV5640_USER_ReadReg(uint16_t devAddr, uint16_t Reg,
                                          uint8_t *pData, uint16_t Length) {
    if (HAL_I2C_Mem_Read(&hi2c1, devAddr, Reg, I2C_MEMADD_SIZE_16BIT, pData,
                         Length, HAL_MAX_DELAY) == HAL_OK) {
        return OV5640_OK;
    }

    return OV5640_ERROR;
}

/**
  user_ov5640_IO.Address = OV5640_I2C_ADDR;
  user_ov5640_IO.Init = OV5640_USER_Init;
  user_ov5640_IO.DeInit = OV5640_USER_DeInit;
  user_ov5640_IO.WriteReg = OV5640_USER_WriteReg;
  user_ov5640_IO.ReadReg = OV5640_USER_ReadReg;
  user_ov5640_IO.GetTick = OV5640_USER_GetTick;
  OV5640_RegisterBusIO(&user_ov5640_OBJ, &user_ov5640_IO);
 */
static OV5640_IO_t user_ov5640_IO;
static OV5640_Object_t user_ov5640_OBJ;
#ifdef __cplusplus
}
#endif

#endif
