#include "stm32f0xx_hal.h"
#include <string.h>
#include <stdio.h>
#include "GBC.h"
#include "GBA.h"
#include "sd.h"
#include "main.h"
#include "uart.h"

void Init_All(void);

int main(void) {
  Init_All();   // Add any additional initialization here

  GBA_test();   // Test GBA stuff here

  GBC_test();

  sd_test();

  while(1){}
}

void Init_All(){
  HAL_Init();
  SystemClock_Config();

  __HAL_RCC_GPIOA_CLK_ENABLE();  // Enable GPIOA clock
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_UART1_Init();  
  UART1_Init();  

  GPIO_InitTypeDef RDWR = {0};  // Upper address bus
  RDWR.Pin = GPIO_PIN_0 | GPIO_PIN_9 | GPIO_PIN_10;
  RDWR.Mode = GPIO_MODE_OUTPUT_PP;  // Push-pull output
  RDWR.Pull = GPIO_NOPULL;
  RDWR.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(GPIOB, &RDWR);

  shift_enable();

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);    // Read disable
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);     // Write disable

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);    // Reset cart
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
  HAL_Delay(10);
}
