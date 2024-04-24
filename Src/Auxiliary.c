/*
 * Auxiliary.c
 *
 *  Created on: Apr 12, 2024
 *      Author: yty
 */
 #include "cmsis_os.h"
 #include "Auxiliary.h"
 
void SendInfo2Uart(UART_HandleTypeDef* pHUart,const unsigned char* pData, uint16_t uLength)
{
	while(HAL_OK != HAL_UART_Transmit_IT(pHUart,pData,uLength))
	{
		osDelay(10);
	}
	UpdateUartSendInfo(pHUart,uLength);
}

/// 让板子休眠
void SetBoardSleep()
{
	HAL_SuspendTick();
	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  HAL_ResumeTick();
}

/// 更改模型
void ChangeMode(STM_BOARD_STATE emState)
{
	switch(emState)
	{
		case SLEEP_YTY:
			break;
	}
}

extern uint8_t GetCPUUsage(void);

/// 获取状态
STMSTATUS GetStatus(void)
{
	static STMSTATUS S_LOCAL={0,0,0,0};
	S_LOCAL.unRamFree = xPortGetFreeHeapSize();
	S_LOCAL.unCPURate = GetCPUUsage();
	return(S_LOCAL);
}
