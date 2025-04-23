#include "rtc.h"

RTC_HandleTypeDef hrtc;

void MX_RTC_Init(void)
{
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

  HAL_RTC_Init(&hrtc);
}

void set_default_rtc(void)
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  sTime.Hours   = 12;
  sTime.Minutes = 0;
  sTime.Seconds = 0;

  sDate.WeekDay = RTC_WEEKDAY_TUESDAY;
  sDate.Month   = RTC_MONTH_APRIL;
  sDate.Date    = 22;
  sDate.Year    = 25;

  HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

void rtc_check_and_set(void)
{
  RTC_DateTypeDef sDate = {0};
  RTC_TimeTypeDef sTime = {0};

  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN); // Must read date after time

  if (sDate.Year < 23) {
    set_default_rtc();  // Set to 2025-04-22 12:00:00
  }
}

// Provide time for FatFs
DWORD get_fattime(void)
{
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;

  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

  return ((DWORD)(sDate.Year + 2000 - 1980) << 25)
        | ((DWORD)sDate.Month << 21)
        | ((DWORD)sDate.Date << 16)
        | ((DWORD)sTime.Hours << 11)
        | ((DWORD)sTime.Minutes << 5)
        | ((DWORD)sTime.Seconds >> 1);
}