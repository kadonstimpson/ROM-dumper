#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"
#include "GBC.h"
#include "uart.h"
#include <stdio.h>

static uint8_t data_bus_mode = 0xFF;  // 0 = input, 1 = output, 0xFF = uninitialized

void GBC_test(void){
    // GBC_read_bank(0x0000);
    test_bank_switch();
}

void shift_enable(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // Push-pull output
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET); // Write PA2 High

}

void GBC_write_addr(uint32_t addr){

    GPIO_InitTypeDef ADDR_H = {0};  // Upper address bus
    ADDR_H.Pin = GBC_ADDR;
    ADDR_H.Mode = GPIO_MODE_OUTPUT_PP;  // Push-pull output
    ADDR_H.Pull = GPIO_NOPULL;
    ADDR_H.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOC, &ADDR_H);

    GPIOC->ODR = addr & 0xFFFF;
}

uint8_t GBC_read(void){

    return GPIOB->IDR & 0xFF;     // Read input
}

void GBC_read_bank(uint32_t start_addr){    // In progress, dumps one bank to serial 

    GBC_set_data_input();

    char buf[16];

    for(int i = start_addr; i < start_addr + 0x4000; i++)
    {
        GBC_write_addr(i);
        for (volatile int d = 0; d < 30; d++) __NOP();  // ~500 ns delay
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);    // Read enable
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);    // Pull /CS low
        for (volatile int d = 0; d < 30; d++) __NOP();  // ~500 ns delay
        // HAL_Delay(1);

        uint8_t byte = GBC_read();
        sprintf(buf, "%02X", byte);
        send_serial(buf);
        // if ((i & 0x0F) == 0x0F) send_serial("\r\n");  // newline every 16 bytes

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);    // Read disable
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);    // Pull /CS high
    }
}

void GBC_write_data(uint8_t byte){
    GBC_set_data_output();                                  // Ensure clk enable, data bus output mode
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);    // Ensure read disabled
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);   // Write enable
    GPIOB->ODR = (GPIOB->ODR & ~0x00FF) | (byte & 0xFF);    // Write byte to PB0-PB7
    for (volatile int d = 0; d < 30; d++) __NOP();          // ~500 ns delay
    // HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);     // Write disable
}

void GBC_set_data_output(void) {
    if (data_bus_mode != 1) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GBC_DAT;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        data_bus_mode = 1;
    }
}

void GBC_set_data_input(void) {
    if (data_bus_mode != 0) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GBC_DAT;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        data_bus_mode = 0;
    }
}

void test_bank_switch(void){
    char buf[32];

    for (uint8_t bank = 0; bank < 4; bank++) {
        // Switch bank with proper timing
        GBC_write_addr(0x3000);       // Bit 8
        GBC_write_data((bank >> 8) & 0x01);
        HAL_Delay(1);
        GBC_write_addr(0x2000);
        GBC_write_data(bank);
        GBC_set_data_input();          //  <‑‑ release PB0‑PB7
        HAL_Delay(5);  // Extended delay after bank switch (5ms)
        
        // Read first 16 bytes of this bank
        send_serial("\r\nBank ");
        sprintf(buf, "%d: ", bank);
        send_serial(buf);
        
        for (int addr = 0x4000; addr < 0x4010; addr++) {
            GBC_write_addr(addr);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
            for (volatile int d = 0; d < 100; d++) __NOP();  // Longer read delay
            uint8_t byte = GBC_read();
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
            
            sprintf(buf, "%02X ", byte);
            send_serial(buf);
        }

        // GBC_read_bank(0x4000);
    }

    printf("Hello from F072 @ %lu Hz\r\n",
        HAL_RCC_GetHCLKFreq());
 
    for (int i = 0; i < 256; ++i) {
        printf("%02X ", i);
        if ((i & 0x0F) == 0x0F) printf("\r\n");
    }
}

void GBC_dump_cart(void){

}