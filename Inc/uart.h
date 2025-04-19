#pragma once
#include "stm32f0xx_hal.h"

extern UART_HandleTypeDef huart1;

void GPIO_UART1_Init(void);
void UART1_Init(void);
void send_serial(const char* msg);