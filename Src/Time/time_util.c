/*
 ******************************************************************************
 * @file           : time_util.c
 * @brief          : Civil calendar + Beijing (UTC+8) timezone helpers
 ******************************************************************************
 */
#include <stddef.h>
#include "Time/time_util.h"

int32_t TimeUtil_DaysFromCivil(int32_t year, uint32_t month, uint32_t day)
{
  int32_t era;
  uint32_t year_of_era;
  uint32_t day_of_year;
  uint32_t day_of_era;
  int32_t adjusted_month;

  year -= (month <= 2U) ? 1 : 0;
  era = (year >= 0) ? (year / 400) : ((year - 399) / 400);
  year_of_era = (uint32_t)(year - era * 400);
  adjusted_month = (int32_t)month + ((month > 2U) ? -3 : 9);
  day_of_year = (uint32_t)((153 * adjusted_month + 2) / 5) + day - 1U;
  day_of_era = year_of_era * 365U + year_of_era / 4U - year_of_era / 100U +
               day_of_year;
  return era * 146097 + (int32_t)day_of_era - 719468;
}

void TimeUtil_CivilFromDays(int32_t days, int32_t *year, uint32_t *month,
                            uint32_t *day)
{
  int32_t era;
  uint32_t day_of_era;
  uint32_t year_of_era;
  int32_t y;
  uint32_t day_of_year;
  uint32_t month_prime;
  int32_t calculated_month;

  if ((year == NULL) || (month == NULL) || (day == NULL)) {
    return;
  }
  days += 719468;
  era = (days >= 0) ? (days / 146097) : ((days - 146096) / 146097);
  day_of_era = (uint32_t)(days - era * 146097);
  year_of_era = (day_of_era - day_of_era / 1460U + day_of_era / 36524U -
                 day_of_era / 146096U) /
                365U;
  y = (int32_t)year_of_era + era * 400;
  day_of_year = day_of_era -
                (365U * year_of_era + year_of_era / 4U -
                 year_of_era / 100U);
  month_prime = (5U * day_of_year + 2U) / 153U;
  *day = day_of_year - (153U * month_prime + 2U) / 5U + 1U;
  calculated_month = (int32_t)month_prime + ((month_prime < 10U) ? 3 : -9);
  *month = (uint32_t)calculated_month;
  *year = y + ((*month <= 2U) ? 1 : 0);
}

uint32_t TimeUtil_CivilToUnix(int32_t year, uint32_t month, uint32_t day,
                              uint32_t hour, uint32_t minute, uint32_t second)
{
  const int32_t days = TimeUtil_DaysFromCivil(year, month, day);
  return (uint32_t)((uint64_t)(uint32_t)days * TIME_SECONDS_PER_DAY) +
         hour * 3600U + minute * 60U + second;
}

uint8_t TimeUtil_UtcToBeijing(uint32_t unix_utc, uint32_t *beijing_unix)
{
  int64_t beijing;

  if (beijing_unix == NULL) {
    return 0U;
  }
  beijing = (int64_t)unix_utc + (int64_t)TIME_BEIJING_OFFSET_SECONDS;
  if (beijing < 0) {
    return 0U;
  }
  *beijing_unix = (uint32_t)beijing;
  return 1U;
}

uint8_t TimeUtil_BeijingToUtc(uint32_t beijing_unix, uint32_t *unix_utc)
{
  int64_t utc;

  if (unix_utc == NULL) {
    return 0U;
  }
  utc = (int64_t)beijing_unix - (int64_t)TIME_BEIJING_OFFSET_SECONDS;
  if (utc < 0) {
    return 0U;
  }
  *unix_utc = (uint32_t)utc;
  return 1U;
}
