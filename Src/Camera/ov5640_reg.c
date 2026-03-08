/**
  ******************************************************************************
  * @file    ov5640_reg.c
  * @author  MCD Application Team
  * @brief   本文件提供控制 OV5640 摄像头驱动的寄存器单元函数。
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

/* 头文件包含 ------------------------------------------------------------------*/
#include "ov5640_reg.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup Components
  * @{
  */

/** @addtogroup OV5640
  * @brief     本文件提供驱动 OV5640 摄像头传感器所需的一组函数。
  * @{
  */

/**
  * @brief  读取 OV5640 组件寄存器
  * @param  ctx 组件上下文
  * @param  reg 要读取的寄存器
  * @param  pdata 数据缓冲区指针
  * @param  length 要读取的数据长度
  * @retval 组件状态
  */
int32_t ov5640_read_reg(ov5640_ctx_t *ctx, uint16_t reg, uint8_t *pdata, uint16_t length)
{
  return ctx->ReadReg(ctx->handle, reg, pdata, length);
}

/**
  * @brief  写入 OV5640 组件寄存器
  * @param  ctx 组件上下文
  * @param  reg 要写入的寄存器
  * @param  pdata 数据缓冲区指针
  * @param  length 要写入的数据长度
  * @retval 组件状态
  */
int32_t ov5640_write_reg(ov5640_ctx_t *ctx, uint16_t reg, uint8_t *data, uint16_t length)
{
  return ctx->WriteReg(ctx->handle, reg, data, length);
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
