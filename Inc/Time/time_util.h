/*
 ******************************************************************************
 * @file           : time_util.h
 * @brief          : Civil calendar + Beijing (UTC+8) timezone helpers
 ******************************************************************************
 */
#ifndef STM32TOOLS_TIME_UTIL_H
#define STM32TOOLS_TIME_UTIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_SECONDS_PER_DAY 86400UL
/** Beijing relative to UTC: +8 hours. */
#define TIME_BEIJING_OFFSET_MINUTES 480
#define TIME_BEIJING_OFFSET_SECONDS                                        \
  ((int32_t)TIME_BEIJING_OFFSET_MINUTES * 60)

int32_t TimeUtil_DaysFromCivil(int32_t year, uint32_t month, uint32_t day);
void TimeUtil_CivilFromDays(int32_t days, int32_t *year, uint32_t *month,
                            uint32_t *day);
uint32_t TimeUtil_CivilToUnix(int32_t year, uint32_t month, uint32_t day,
                              uint32_t hour, uint32_t minute, uint32_t second);

/** UTC unix → Beijing wall-clock unix; returns 0 on failure. */
uint8_t TimeUtil_UtcToBeijing(uint32_t unix_utc, uint32_t *beijing_unix);
/** Beijing wall-clock unix → UTC unix; returns 0 on failure. */
uint8_t TimeUtil_BeijingToUtc(uint32_t beijing_unix, uint32_t *unix_utc);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_TIME_UTIL_H */
