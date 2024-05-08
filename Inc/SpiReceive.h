/**
 ******************************************************************************
  * @file           : SpiReceive.h
  * @brief          : Header for SpiReceive.c file.
  *                   此文件为接收SPI数据.
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
#ifndef __SPI_RECEIVE_H
#define __SPI_RECEIVE_H
#include "stm32f1xx_hal.h"

/// 定义串口接收数据量用于调试
typedef struct _Spi_IO_Info
{
	uint64_t unReciveCount;
	uint64_t unSendCount;
	uint64_t unDealCount;
}SpiIOInfo;

/// 定义串口传输
typedef struct _Spi_Queue_Info
{
	uint8_t* pBuffer;
	uint16_t nLength;
}SpiQueueInfo;

#endif
