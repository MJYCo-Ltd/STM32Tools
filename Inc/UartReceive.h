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

typedef void (*ReceiveUartCallback)(UART_HandleTypeDef*,const uint8_t* pData,uint16_t nDataSize);

/// 初始化串口数量
void InitUartCount(uint8_t unMaxUartSize);

/**
 * 增加串口
 */
uint8_t AddUart(UART_HandleTypeDef* pHUart,ReceiveUartCallback pCallback);

/**
 *接收指定id的串口数据
 */
void ReciveUartInfo(uint8_t uId);

/**
 *定时处理
 */
void ProcessUart(uint32_t clock);

#endif
