/*
 * Auxiliary.c
 *
 *  Created on: Apr 12, 2024
 *      Author: yty
 */
#include "string.h"
#include "cmsis_os.h"
#include "Auxiliary.h"
#include "UartReceive.h"

/// ������Ϣ������
void SendDebugInfo(const unsigned char* pData, uint16_t uLength)
{
	if(GetUartCount() < 1) return;
	UART_HandleTypeDef* pHUart = GetUart(1);
	while(HAL_OK != HAL_UART_Transmit_DMA(pHUart,pData,uLength))
	{
		osDelay(10);
	}
	UpdateUartSendInfo(pHUart,uLength);
}

/// ����ռ�
void* RequestSpace(size_t unSize)
{
	void* pBuffer = pvPortMalloc(unSize);
	if(NULL != pBuffer)
	{
		memset(pBuffer,0,unSize);
	}
	
	return(pBuffer);
}

/// ���տռ�
void RecycleSpace(void* pBuffer)
{
	vPortFree(pBuffer);
}
/// �ð�������
void SetBoardSleep()
{
	HAL_SuspendTick();
	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  HAL_ResumeTick();
}

/// ����ģ��
void ChangeMode(STM_BOARD_STATE emState)
{
	switch(emState)
	{
		case SLEEP_YTY:
			break;
	}
}

extern uint8_t GetCPUUsage(void);

/// ��ȡ״̬
STMSTATUS GetStatus(void)
{
	static STMSTATUS S_LOCAL={0,0,0,0};
	S_LOCAL.unRamFree = xPortGetFreeHeapSize();
//	S_LOCAL.unCPURate = GetCPUUsage();
	return(S_LOCAL);
}
