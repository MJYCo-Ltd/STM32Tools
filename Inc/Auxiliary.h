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
#ifndef __YTY_AUXILIARY_H_
#define __YTY_AUXILIARY_H_
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    LP_MODE_STOP=0,
    LP_MODE_STANDBY
} LOW_POWER_MODE;

typedef struct {
    size_t unRamTotal;     /// dynamic heap capacity, bytes (not total MCU RAM)
    size_t unRamFree;      /// free dynamic heap bytes
    uint8_t unCPURate;      /// CPU使用率
    uint32_t unCPUFrequency; /// CPU主频 MHZ
} STMSTATUS;

#include "IOInfo.h" /* Backward-compatible type exposure. */

typedef enum {
    AUXILIARY_OK = 0,
    AUXILIARY_ERR_PARAM,
    AUXILIARY_ERR_UNCONFIGURED,
    AUXILIARY_ERR_IO
} AuxiliaryStatus;

typedef AuxiliaryStatus (*AuxiliaryRestoreClock)(void *context);
typedef struct {
    /* STM32 HAL RTC/UART handles. Kept opaque so this public header does not
     * require a product main.h or a particular STM32 family HAL header. */
    void *rtc;
    void *debug_uart; /* NULL disables debug output; never assumes UART 1. */
    AuxiliaryRestoreClock restore_clock;
    void *context;
} AuxiliaryConfig;

/* Configure once at boot before concurrent use. Configuration is copied;
 * handles and context are borrowed. NULL resets to unconfigured/disabled. */
void Auxiliary_Configure(const AuxiliaryConfig *config);
AuxiliaryStatus Auxiliary_EnterStop(void);
AuxiliaryStatus Auxiliary_EnterLowPower(LOW_POWER_MODE mode, uint32_t counter,
                                        uint32_t clock);
AuxiliaryStatus Auxiliary_LastError(void);

/* Low-power preconditions: task context, wake source selected, no pending work.
 * The caller coordinates RTOS tick suppression/time accounting, peripheral
 * ownership and the watchdog budget. These are NOT whole-system sleep APIs. */

/**
 * 发送调试信息
 *@pragma pData   要打印的字符串
 *@pragma unLength 要打印的字符串长度
 */
void SendDebugInfo(const uint8_t *pData, uint16_t unLength);

/**
 * 请求新的空间
 *@pragma unSize 要开辟空间的字节大小
 *@return 如果剩余空间大小小于申请的空间返回NULL
 *@attention 返回的空间都进行了置零操作
 */
void *RequestSpace(size_t unSize);

/**
 * 回收空间
 */
void RecycleSpace(void *pBuffer);

/**
 * @brief 获取flash
 * @return
 */
/* Legacy declaration only: STM32Tools has no ReadFlash implementation.
 * Prefer the explicit Flash backend API; a legacy product may supply this. */
const uint8_t *ReadFlash(void);

/**
 * @brief 进入休眠模式
 */
void Enter_Sleep(void);

/**
 * @brief 进入停止模式
 */
void Enter_Stop(void);

/**
 * @brief 进入低功耗模式
 * @param 低功耗模式
 * @param WakeUpCounter 定时个数
 * @param WakeUpClock  定时周期
 */
void EnterLowPowerMode(LOW_POWER_MODE mode, uint32_t WakeUpCounter,
                       uint32_t WakeUpClock);

/**
 *获取单片机状态
 */
STMSTATUS GetStatus(void);
#endif //__YTY_AUXILIARY_H
