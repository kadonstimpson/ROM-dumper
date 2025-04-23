#include "stm32f0xx_hal.h"
#include <string.h>
#include <stdio.h>
#include "sd.h"
#include "main.h"
#include "uart.h"



int main(void) 
{
  SystemClock_Config();

  // PC9 LED init
  RCC->AHBENR |= RCC_AHBENR_GPIOCEN;  // Enable GPIOC clock

  GPIO_InitTypeDef gpioInit = {
    .Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9,
    .Mode = GPIO_MODE_OUTPUT_PP,
    .Pull = GPIO_NOPULL,
    .Speed = GPIO_SPEED_FREQ_LOW
  };
  HAL_GPIO_Init(GPIOC, &gpioInit);

  GPIO_UART1_Init();
  UART1_Init();

  printf("hi\n");
  sd_test();

  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);  // Turn on LED

}

