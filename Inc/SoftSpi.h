/**
 ******************************************************************************
  * @file           : SoftSpi.h
  * @brief          : 此文件为SPI的系统实现.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 mjytech.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *  Created on: May 7, 2024
  *      Author: wshys
  ******************************************************************************
 */
#ifndef __SOFT_SPI_H
#define __SOFT_SPI_H
#include <stdint.h>

/**
 * @brief  SPI发送数据函数，用户可以根据自己的需要重写此函数
 */
void SPI_Write(uint8_t data);

/**
 * @brief  SPI发送数据函数，用户可以根据自己的需要重写此函数
 */
void SPI_WriteBuffer(const uint8_t *pBuffer, uint16_t unLength);

#endif
