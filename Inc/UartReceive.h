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
#ifndef __YTY_UART_RECEIVE_H_
#define __YTY_UART_RECEIVE_H_
#include "Auxiliary.h"
/// 定义串口传输
typedef struct _Uart_Queue_Info
{
	uint8_t* pBuffer;
	uint16_t nLength;
}UartQueueInfo;

typedef void (*ReceiveUartCallback)(UART_HandleTypeDef*,uint8_t* pData,uint16_t nLength);

/**
 * 初始化串口数量
 * @attention 此数量用于开辟空间
 */
void InitUartCount(uint8_t unMaxUartSize);

/**
 * 添加串口
 */
uint8_t AddUart(UART_HandleTypeDef* pHUart,ReceiveUartCallback pCallback);
UART_HandleTypeDef* GetUart(uint8_t uId);

/**
 * 获取串口接收数据信息
 */
const IOInfo* GetUartIOInfo(uint8_t uId);

/**
 * 更新串口发送数据
 */
void UpdateUartSendInfo(UART_HandleTypeDef* pHUart,uint16_t unLength);

/**
 * 接收指定id的串口数据
 */
void BeginReceiveUartInfo(uint8_t uId);
void StopReceiveUartInfo(uint8_t uId);

/**
 * 定时处理
 */
void ProcessUart(void);

/**
 * 获取串口数据的数量
 */
uint8_t GetUartCount(void);

#endif
