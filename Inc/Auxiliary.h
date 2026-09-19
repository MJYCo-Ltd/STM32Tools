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
#ifndef STM32TOOLS_AUXILIARY_H
#define STM32TOOLS_AUXILIARY_H

#include <stddef.h>
#include <stdint.h>
#include "IOStatistics.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { LP_MODE_STOP = 0, LP_MODE_STANDBY } LOW_POWER_MODE;
typedef enum {
  AUXILIARY_OK = 0,
  AUXILIARY_ERR_PARAM,
  AUXILIARY_ERR_UNCONFIGURED,
  AUXILIARY_ERR_IO,
  AUXILIARY_ERR_CLOCK
} AuxiliaryResult;
/* The audit branch used these names for the same checked platform contract. */
typedef AuxiliaryResult AuxiliaryStatus;

enum { STMSTATUS_HEAP_VALID = 1U, STMSTATUS_CPU_FREQUENCY_VALID = 2U };
/** These are heap bytes, NOT all physically unused MCU RAM. CPU load is not
 * measured here. The wider fields intentionally change the legacy struct ABI;
 * rebuild callers and do not serialize this native structure as a wire format.
 */
typedef struct {
  size_t unRamTotal;
  size_t unRamFree;
  uint8_t unCPURate;
  uint32_t unCPUFrequency; /* MHz; retained unit for source compatibility */
  uint8_t valid_fields;
} STMSTATUS;

typedef AuxiliaryResult (*AuxiliaryClockRestore)(void *context);
typedef AuxiliaryClockRestore AuxiliaryRestoreClock;
typedef struct {
  void *rtc;        /* RTC_HandleTypeDef*, borrowed; NULL disables timed sleep */
  void *debug_uart; /* UART_HandleTypeDef*, borrowed; NULL disables debug TX */
  AuxiliaryClockRestore restore_clock;
  void *context;
} AuxiliaryConfig;

/** Configure once during startup, before concurrent users/interrupts. NULL
 * removes the binding. No default UART, RTC symbol or board clock is selected.
 * STM32 HAL low-power APIs are platform-specific; this implementation uses the
 * existing F4-style HAL power/RTC API. Other STM32 families need matching ports.
 */
void Auxiliary_Configure(const AuxiliaryConfig *config);
/* Diagnostic for legacy void low-power APIs; last caller wins. Configure and
 * checked/legacy low-power calls share one serialized owner. */
AuxiliaryStatus Auxiliary_LastError(void);
AuxiliaryResult Auxiliary_SendDebug(const uint8_t *data, uint16_t length);
void SendDebugInfo(const uint8_t *data, uint16_t length);
void *RequestSpace(size_t bytes);
void RecycleSpace(void *buffer);
STMSTATUS GetStatus(void);

/** Low-level mechanisms only. The caller owns RTOS tick suppression/time
 * compensation, wake ISR setup, peripheral quiescence and IWDG sleep budget.
 * Call only from the board's coordinated low-power path, never arbitrarily
 * from a running RTOS task. No mode is entered if RTC setup fails.
 */
AuxiliaryResult Auxiliary_EnterStop(void);
AuxiliaryResult Auxiliary_EnterLowPower(LOW_POWER_MODE mode, uint32_t counter, uint32_t clock);
void Enter_Sleep(void);
void Enter_Stop(void); /* legacy checked internally; use checked API for errors */
void EnterLowPowerMode(LOW_POWER_MODE mode, uint32_t counter, uint32_t clock);

#ifdef __cplusplus
}
#endif
#endif
