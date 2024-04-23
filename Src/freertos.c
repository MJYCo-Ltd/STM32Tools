/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "Auxiliary.h"
#include "UartReceive.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CALCULATION_PERIOD 1000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
TaskHandle_t xIdleHandle=NULL;
TickType_t osCPU_IdleStartTime=0;
TickType_t osCPU_IdleSpentTime=0;
TickType_t osCPU_IdleTotalTime=0;
uint8_t  osCPU_Usege=0;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Timer */
osTimerId_t TimerHandle;
const osTimerAttr_t Timer_attributes = {
  .name = "Timer"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
uint8_t GetCPUUsage(void);
extern UART_HandleTypeDef huart1;
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void Timer_100ms(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationIdleHook(void);
void vApplicationTickHook(void);

/* USER CODE BEGIN 2 */
void vApplicationIdleHook( void )
{
   if(NULL == xIdleHandle)
	 {
		 xIdleHandle = xTaskGetCurrentTaskHandle();
	 }
}
/* USER CODE END 2 */

/* USER CODE BEGIN 3 */
void vApplicationTickHook( void )
{
	static uint16_t S_LOCAL_TICK=0;
	if(S_LOCAL_TICK++ > CALCULATION_PERIOD)
	{
		S_LOCAL_TICK = 0;
		if(osCPU_IdleTotalTime > CALCULATION_PERIOD)
		{
			osCPU_IdleTotalTime = CALCULATION_PERIOD;
		}
		osCPU_Usege = (100-(osCPU_IdleTotalTime*100)/CALCULATION_PERIOD);
		osCPU_IdleTotalTime = 0;
	}
}
/* USER CODE END 3 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of Timer */
  TimerHandle = osTimerNew(Timer_100ms, osTimerPeriodic, NULL, &Timer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  osTimerStart(TimerHandle,1000U);
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Timer_100ms function */
void Timer_100ms(void *argument)
{
  /* USER CODE BEGIN Timer_100ms */
	ProcessUart(xTaskGetTickCount());
	static char test[76]="";
	sprintf(test,"cpu usage %d\n",GetCPUUsage());
	SendInfo2Uart(&huart1,(const unsigned char*)test,strlen(test));
	osDelay(10);
	for(uint8_t index=0;index<GetUartCount();++index)
	{
		const UartIOInfo* pIOInfo = GetUartIOInfo(index);
		sprintf(test,"%d Uart Recive %llu bytes Deal %llu bytes\nSend %llu bytes\n",index,
		pIOInfo->unDealCount,pIOInfo->unReciveCount,pIOInfo->unSendCount);
	  SendInfo2Uart(&huart1,(const unsigned char*)test,strlen(test));
	}

  /* USER CODE END Timer_100ms */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void StartIdleMonitor(void)
{
	if(xTaskGetCurrentTaskHandle()==xIdleHandle)
	{
		osCPU_IdleStartTime = xTaskGetTickCountFromISR();
	}
}

void EndIdleMonitor(void)
{
	if(xTaskGetCurrentTaskHandle()==xIdleHandle)
	{
		osCPU_IdleSpentTime = xTaskGetTickCountFromISR() - osCPU_IdleStartTime;
		osCPU_IdleTotalTime += osCPU_IdleSpentTime;
	}
}

uint8_t GetCPUUsage(void)
{
	return(osCPU_Usege);
}
/* USER CODE END Application */

