/**
 ******************************************************************************
  * @file           : UartReceive.h
  * @brief          : Header for UartReceive.c file.
  *                   此文件为接收串口数据.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 mjytech.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *  Created on: Apr 10, 2024
  *      Author: wshys
  ******************************************************************************
 */
#ifndef __UART_RECEIVE_H
#define __UART_RECEIVE_H
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"

/// 定义串口接收数据量用于调试
typedef struct _Uart_IO_Info
{
	uint64_t unReciveCount;
	uint64_t unSendCount;
	uint64_t unDealCount;
}UartIOInfo;

typedef uint16_t ReceiveCalType;

typedef void (*ReceiveUartCallback)(UART_HandleTypeDef*,const uint8_t* pData,ReceiveCalType nDataSize);

/// 初始化串口数量
void InitUartCount(uint8_t unMaxUartSize);

/**
 * 增加串口
 */
uint8_t AddUart(UART_HandleTypeDef* pHUart,ReceiveUartCallback pCallback);

/**
 * 获取串口接收数据信息
 */
const UartIOInfo* GetUartIOInfo(uint8_t uId);

/**
 * 更新串口发送数据
 */
void UpdateUartSendInfo(UART_HandleTypeDef* pHUart,uint16_t unLength);

/**
 * 接收指定id的串口数据
 */
void BeginReciveUartInfo(uint8_t uId);

/**
 * 定时处理
 */
void ProcessUart(TickType_t clock);

/**
 * 获取串口数据的数量
 */
uint8_t GetUartCount(void);

#endif
