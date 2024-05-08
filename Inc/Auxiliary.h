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
#ifndef __YTY_AUXILIARY_H_
#define __YTY_AUXILIARY_H_
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

///��������
typedef struct
{
	uint64_t unReciveCount; /// ���յ����������� (�ֽ�)
	uint64_t unSendCount;   /// ���͵��������� (�ֽ�)
	uint64_t unDealCount;   /// ������������� (�ֽ�)
}IOInfo;

/**
 * ���͵�����Ϣ
 *@pragma pData   Ҫ��ӡ���ַ���
 *@pragma unLength Ҫ��ӡ���ַ�������
 */
void SendDebugInfo(const unsigned char* pData, uint16_t unLength);

/**
 * �����µĿռ�
 *@pragma unSize Ҫ���ٿռ���ֽڴ�С
 *@return ���ʣ��ռ��СС������Ŀռ䷵��NULL
 *@attention ���صĿռ䶼�������������
 */
void* RequestSpace(size_t unSize);

/**
 * ���տռ�
 */
void RecycleSpace(void* pBuffer);

/**
 *���ĵ�Ƭ��״̬
 */
void ChangeMode(STM_BOARD_STATE emState);

/**
 *��ȡ��Ƭ��״̬
 */
STMSTATUS GetStatus(void);
#endif//__YTY_AUXILIARY_H
