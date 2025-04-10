#include "stm32f0xx_hal.h"
#include <string.h>
#include <stdio.h>

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

#define DATA_PORT GPIOC
#define ADDR_LOW_PORT GPIOC   // PC0-PC15
#define ADDR_HIGH_PORT GPIOB  // PB0-PB7
#define CART_RD_PORT GPIOB
#define CART_RD_PIN GPIO_PIN_10

void GPIO_Init_All(void);
void GPIO_UART1_Init(void);
void UART1_Init(void);
void UART2_Init(void);  // Initialize UART2 for USB debug
void set_address(uint16_t addr);
uint8_t read_data_byte(void);
void send_serial(const char* msg);

int lab1_main(void) {
    HAL_Init();
    GPIO_Init_All();
    UART1_Init();
    UART2_Init();
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

    while (1);
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

void GPIO_UART1_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PA9 = USART1_TX, PA10 = USART1_RX
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; 
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART1; 

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void UART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;

    HAL_UART_Init(&huart1);
}

void UART2_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode - UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = USART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void send_serial(const char* msg) {
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}
