#include "stm32f0xx_hal.h"
#include <string.h>
#include <stdio.h>

UART_HandleTypeDef huart1;

#define DATA_PORT GPIOC
#define ADDR_LOW_PORT GPIOC   // PC0-PC15
#define ADDR_HIGH_PORT GPIOB  // PB0-PB7
#define CART_RD_PORT GPIOB
#define CART_RD_PIN GPIO_PIN_10

void GPIO_Init_All(void);
void GPIO_UART1_Init(void);
void UART1_Init(void);
void set_address(uint16_t addr);
uint8_t read_data_byte(void);
void send_serial(const char* msg);

int lab1_main(void) {
    HAL_Init();
    GPIO_Init_All();
    UART1_Init();
    GPIO_UART1_Init();

    char msg[64];

    for (uint16_t addr = 0x100; addr <= 0x14F; addr++) {
        set_address(addr);

        HAL_GPIO_WritePin(CART_RD_PORT, CART_RD_PIN, GPIO_PIN_RESET);
        HAL_Delay(1);

        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = 0x00FF; // PC0-PC7
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(DATA_PORT, &GPIO_InitStruct);

        uint8_t data = read_data_byte();

        HAL_GPIO_WritePin(CART_RD_PORT, CART_RD_PIN, GPIO_PIN_SET);

        sprintf(msg, "Addr 0x%04X: 0x%02X\r\n", addr, data);
        send_serial(msg);
        HAL_Delay(10);
    }

    while (1) {
        HAL_Delay(100);
        printf("\nUSART Test...");
    }
}

void GPIO_Init_All(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Address bus (PC0-PC15 + PB0-PB7)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = 0xFFFF;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = 0x00FF;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // CART_RD
    GPIO_InitStruct.Pin = CART_RD_PIN;
    HAL_GPIO_Init(CART_RD_PORT, &GPIO_InitStruct);
}

void set_address(uint16_t addr) {
    // 写入 PC0-PC15
    for (int i = 0; i < 16; i++) {
        HAL_GPIO_WritePin(GPIOC, (1 << i), (addr & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

uint8_t read_data_byte(void) {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= (HAL_GPIO_ReadPin(DATA_PORT, (1 << i)) << i);
    }
    return value;
}
