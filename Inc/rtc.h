#pragma once

#include "stm32f0xx_hal.h"
#include "ff.h"

extern RTC_HandleTypeDef hrtc;

void MX_RTC_Init(void);            // RTC peripheral initialization
void set_default_rtc(void);        // Set fixed time (e.g., 2025-04-22 12:00:00)
void rtc_check_and_set(void);      // Only set RTC if year is unset (< 2023)
DWORD get_fattime(void);           // get time for fatFs