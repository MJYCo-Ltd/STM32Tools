/*
 ******************************************************************************
 * @file           : Auxiliary.h
 * @brief          : Header for Auxiliary.c file.
 *                   ���ļ�ΪSTM32�ĸ�������
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
	uint8_t unRamTotal;     /// ram�ܿռ�
	uint8_t unRamFree;      /// ramʣ��ռ�
	uint8_t unCPURate;      /// CPUʹ����
	uint8_t unCPUFrequency; /// CPU��Ƶ
}STMSTATUS;
/**
 * ������Ϣ������
 *@pragma pHUart  ָ�򴮿ھ����ָ��
 *@pragma pData   Ҫ��ӡ���ַ���
 *@pragma unLength Ҫ��ӡ���ַ�������
 */
void SendInfo2Uart(UART_HandleTypeDef* pHUart,const unsigned char* pData, uint16_t unLength);

/**
 *���ĵ�Ƭ��״̬
 */
void ChangeMode(STM_BOARD_STATE emState);

/**
 *��ȡ��Ƭ��״̬
 */
STMSTATUS GetStatus(void);
#endif//__YTY_AUXILIARY_H
