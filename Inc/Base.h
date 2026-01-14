/*
 ******************************************************************************
 * @file           : Base.h
 * @brief          : 此文件为判断是否有freertos的一个公共头文件
 ******************************************************************************
 *
 *  Created on: Dec 27, 2025
 *      Author: yty
 */
#ifndef __YTY_BASE_H_
#define __YTY_BASE_H_

#ifdef USE_FREERTOS
#include "cmsis_os.h"
#define YTY_DELAY_MS(ms) osDelay(ms)
#define YTY_MALLOC(size) pvPortMalloc(size)
#define YTY_FREE(ptr) vPortFree(ptr)
#else
#include <stdlib.h>
#define YTY_DELAY_MS(ms) HAL_Delay(ms)
#define YTY_MALLOC(size) malloc(size)
#define YTY_FREE(ptr) free(ptr)
#endif

#endif //__YTY_BASE_H_
