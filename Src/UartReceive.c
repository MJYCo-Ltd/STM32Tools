/*
 * UartReceive.c
 *
 *  Created on: Apr 10, 2024
 *      Author: yty
 */
#include "cmsis_os.h"
#include "UartReceive.h"

/// 定义串口数据结构体
typedef struct _Uart_Info
{
	UART_HandleTypeDef* pHUart;   /// 串口句柄指针
	ReceiveUartCallback pCallback;/// 串口回调
	uint8_t cReceive;             /// 串口缓冲区
	UartIOInfo allIOInfo;         /// 串口收发数据统计
	osMessageQueueId_t hQueueId;  /// 串口获取消息队列
}Uart_Info;

Uart_Info** pUartInfoArray=NULL;

const BaseType_t UART_BUFFER_LENGTH = 100;


uint8_t uUartIndex=0;

/// 初始化串口数量
void InitUartCount(uint8_t unMaxUartSize)
{
	if (NULL == pUartInfoArray)
	{
		/// 此处程序刚开始，如果没有空间说明芯片不合适
		/// 故没有进行判断指针为空的问题
		pUartInfoArray = (Uart_Info**) pvPortMalloc(sizeof(*pUartInfoArray)*unMaxUartSize);
		memset(pUartInfoArray,0,sizeof(*pUartInfoArray)*unMaxUartSize);
	}
}
/**
 * 创建一个接收串口的缓冲区
 */
uint8_t AddUart(UART_HandleTypeDef* pHUart,ReceiveUartCallback pCallback)
{
	if(NULL == pUartInfoArray)
	{
		return(0);
	}

	Uart_Info* pUartInfo = (Uart_Info*)pvPortMalloc(sizeof(*pUartInfo));

	if(NULL == pUartInfo)
	{
		return(0);
	}
	else
	{
		memset(pUartInfo,0,sizeof(*pUartInfo));
		pUartInfo->hQueueId = osMessageQueueNew( UART_BUFFER_LENGTH, sizeof(uint8_t),NULL);
		///绑定
		pUartInfo->pHUart = pHUart;
		pUartInfo->pCallback = pCallback;
		pUartInfoArray[uUartIndex] = pUartInfo;
	}

	return(++uUartIndex);
}

///开始接收数据
void BeginReciveUartInfo(uint8_t uId)
{
	if(uId <1 ) return;
	Uart_Info* pUartInfo = pUartInfoArray[uId-1];
	HAL_UART_Receive_IT(pUartInfo->pHUart, &pUartInfo->cReceive, 1);
}

/// 中断异步接收回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *pHUart)
{
	for(uint8_t index=0;index<uUartIndex;++index)
	{
		Uart_Info* pUartInfo = pUartInfoArray[index];
		if(pHUart == pUartInfo->pHUart)
		{
			++pUartInfo->allIOInfo.unReciveCount;
			osMessageQueuePut(pUartInfo->hQueueId,&pUartInfo->cReceive,NULL,NULL);
			/// 重新申请中断
			HAL_UART_Receive_IT(pUartInfo->pHUart, &pUartInfo->cReceive, 1);
		}
	}
}

/// 定时处理数据
void ProcessUart(void)
{
	static uint8_t LOCAL_BYTE;
	for(uint8_t index=0;index<uUartIndex;++index)
	{
		Uart_Info* pUartInfo = pUartInfoArray[index];
		if(NULL != pUartInfo->pHUart)
		{
			osMessageQueueGet(pUartInfo->hQueueId,&LOCAL_BYTE,NULL,NULL);
			pUartInfo->pCallback(pUartInfo->pHUart,LOCAL_BYTE);
		}
	}
}

/// 获取串口接收数据信息
const UartIOInfo* GetUartIOInfo(uint8_t uId)
{
	if(uId <= uUartIndex)
	{
		Uart_Info* pUartInfo = pUartInfoArray[uId];
		if(NULL != pUartInfo)
		{
			return(&pUartInfo->allIOInfo);
		}
	}
	
	return (NULL);
}

/// 更新串口发送数据
void UpdateUartSendInfo(UART_HandleTypeDef* pHUart,uint16_t unLength)
{
	for(uint8_t index=0;index<uUartIndex;++index)
	{
		Uart_Info* pUartInfo = pUartInfoArray[index];
		if(pHUart == pUartInfo->pHUart)
		{
			pUartInfo->allIOInfo.unSendCount += unLength;
		}
	}
}

/// 获取管理里面的串口数量
uint8_t GetUartCount(void)
{
	return(uUartIndex);
}
