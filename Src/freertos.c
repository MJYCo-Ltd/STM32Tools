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
#include "UartReceive.h"
#include "Auxiliary.h"
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

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
uint8_t GetCPUUsage(void);
extern UART_HandleTypeDef huart3;
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationIdleHook(void);
void vApplicationTickHook(void);

/* USER CODE BEGIN 2 */
void vApplicationIdleHook( void )
{
	SendInfo2Uart(&huart3,"44\n",3);
   if(NULL == xIdleHandle)
	 {
		 SendInfo2Uart(&huart3,"55\n",3);
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

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
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
		static char test[40]="";
		//sprintf(test,"cpu usage %d\n",GetCPUUsage());
		SendInfo2Uart(&huart3,test,15);
		ProcessUart(HAL_GetTick());
    HAL_Delay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void StartIdleMonitor(void)
{
	SendInfo2Uart(&huart3,"00\n",3);
	if(xTaskGetCurrentTaskHandle()==xIdleHandle)
	{
		SendInfo2Uart(&huart3,"11\n",3);
		osCPU_IdleStartTime = xTaskGetTickCountFromISR();
	}
}

void EndIdleMonitor(void)
{
	SendInfo2Uart(&huart3,"22\n",3);
	if(xTaskGetCurrentTaskHandle()==xIdleHandle)
	{
		SendInfo2Uart(&huart3,"33\n",3);
		osCPU_IdleSpentTime = xTaskGetTickCountFromISR() - osCPU_IdleStartTime;
		osCPU_IdleTotalTime += osCPU_IdleSpentTime;
	}
}

uint8_t GetCPUUsage(void)
{
	return(osCPU_Usege);
}
/* USER CODE END Application */

