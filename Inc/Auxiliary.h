/*
 ******************************************************************************
 * @file           : Auxiliary.h
 * @brief          : Header for Auxiliary.c file.
 *                   此文件为STM32的辅助工具
 ******************************************************************************
 *
 *  Created on: Apr 12, 2024
 *      Author: yty
 */
#ifndef __YTY_AUXILIARY_H
#define __YTY_AUXILIARY_H
#include "stm32f1xx_hal.h"

typedef enum
{
	SLEEP_YTY
}STM_BOARD_STATE;

typedef struct
{
	uint8_t unRamTotal;     /// ram总空间
	uint8_t unRamFree;      /// ram剩余空间
	uint8_t unCPURate;      /// CPU使用率
	uint8_t unCPUFrequency; /// CPU主频
}STMSTATUS;
/**
 * 发送信息到串口
 *@pragma pHUart  指向串口句柄的指针
 *@pragma pData   要打印的字符串
 *@pragma unLength 要打印的字符串长度
 */
void SendInfo2Uart(UART_HandleTypeDef* pHUart,const unsigned char* pData, uint16_t unLength);

/**
 *更改单片机状态
 */
void ChangeMode(STM_BOARD_STATE emState);

/**
 *获取单片机状态
 */
STMSTATUS GetStatus(void);
#endif//__YTY_AUXILIARY_H
