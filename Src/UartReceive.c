/*
 * UartReceive.c
 *
 *  Created on: Apr 10, 2024
 *      Author: yty
 */
#include "string.h"
#include "UartReceive.h"

/// 定义串口数据结构体
typedef struct _Uart_Info
{
	UART_HandleTypeDef* pHUart;
	ReceiveUartCallback pCallback;
	uint8_t* pBuffer;
	uint8_t* pReceive;
	ReceiveCalType countReceive;
	TickType_t lastTime;
	UartIOInfo allIOInfo;
}Uart_Info;

Uart_Info** pUartInfoArray=NULL;
uint8_t* pSendBuffer=NULL;

const ReceiveCalType UART_BUFFER_LENGTH = 100;
const TickType_t DELAY_TICK = 100;
const ReceiveCalType UART_ONCE_RECEIVE_BYTES=1;

static const uint8_t YTY_FALSE=0;
static const uint8_t YTY_TRUE=1;


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
		pSendBuffer = (uint8_t*)pvPortMalloc(sizeof(*pSendBuffer)*UART_BUFFER_LENGTH);
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
		uint8_t* pBuffer = (uint8_t*)pvPortMalloc(sizeof(*pBuffer)*UART_BUFFER_LENGTH);
		if(NULL == pBuffer)
		{
			vPortFree(pUartInfo);
			return(0);
		}
		memset(pBuffer,0,sizeof(*pBuffer)*UART_BUFFER_LENGTH);

		///绑定
		pUartInfo->pHUart = pHUart;
		pUartInfo->pBuffer = pBuffer;
		pUartInfo->pReceive = pBuffer;
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
	HAL_UART_Receive_IT(pUartInfo->pHUart, pUartInfo->pReceive, UART_ONCE_RECEIVE_BYTES);
}

/// 通过回调发送数据
void SendData(Uart_Info* pUartInfo,uint8_t clear)
{
	uint8_t *pBuffer = pUartInfo->pBuffer;
	uint8_t *pReceive = pUartInfo->pReceive;
	if(pReceive - pUartInfo->countReceive > pBuffer)
	{
		memcpy(pSendBuffer,pReceive - pUartInfo->countReceive,pUartInfo->countReceive*sizeof(*pReceive));
	}
	else
	{
		ReceiveCalType subSize = pReceive - pBuffer;
		ReceiveCalType preSize = pUartInfo->countReceive - subSize;
		if(preSize>0) memcpy(pSendBuffer,pBuffer + UART_BUFFER_LENGTH - preSize,preSize*sizeof(*pReceive));
		if(subSize>0) memcpy(pSendBuffer+preSize,pBuffer,subSize*sizeof(*pReceive));
	}
	pUartInfo->pCallback(pUartInfo->pHUart,pSendBuffer, pUartInfo->countReceive);
	pUartInfo->allIOInfo.unDealCount += pUartInfo->countReceive;
	pUartInfo->countReceive = 0;
	
	/// 将数据置空，将指针重置
	if(YTY_FALSE != clear)
	{
	  pUartInfo->pReceive = pUartInfo->pBuffer;
	}
}

/// 异步接收回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *pHUart)
{
	for(uint8_t index=0;index<uUartIndex;++index)
	{
		Uart_Info* pUartInfo = pUartInfoArray[index];
		if(pHUart == pUartInfo->pHUart)
		{
			pUartInfo->lastTime = xTaskGetTickCountFromISR();
			pUartInfo->pReceive += UART_ONCE_RECEIVE_BYTES;
			pUartInfo->countReceive += UART_ONCE_RECEIVE_BYTES;
			pUartInfo->allIOInfo.unReciveCount += UART_ONCE_RECEIVE_BYTES;
			
			/// 如果索引越界直接减去缓冲区大小
			if((pUartInfo->pReceive - pUartInfo->pBuffer)>=UART_BUFFER_LENGTH)
			{
				pUartInfo->pReceive -= UART_BUFFER_LENGTH;
			}
			
			/// 一旦要接收的数据加上已接收的数据大于缓冲区，进行处理
			if(pUartInfo->countReceive + UART_ONCE_RECEIVE_BYTES > UART_BUFFER_LENGTH)
			{
				SendData(pUartInfo,YTY_TRUE);
			}

			/// 重新申请中断
			HAL_UART_Receive_IT(pUartInfo->pHUart, pUartInfo->pReceive, UART_ONCE_RECEIVE_BYTES);
		}
	}
}

/// 定时处理数据
void ProcessUart(TickType_t clock)
{
	for(uint8_t index=0;index<uUartIndex;++index)
	{
		Uart_Info* pUartInfo = pUartInfoArray[index];
		if(pUartInfo->countReceive > 0 && clock-pUartInfo->lastTime > DELAY_TICK)
		{
			SendData(pUartInfo,YTY_FALSE);
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
