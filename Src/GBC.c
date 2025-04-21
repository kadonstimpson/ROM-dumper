#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"
#include "GBC.h"
#include "uart.h"
#include <stdio.h>

static uint8_t data_bus_mode = 0xFF;  // 0 = input, 1 = output, 0xFF = uninitialized

void GBC_test(void){
    // GBC_read_bank(0x0000);
    // test_bank_switch();
    GBC_dump_cart();
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

uint8_t GBC_read(uint32_t addr){
    GBC_set_data_input();
    GBC_write_addr(addr);
    for (volatile int d = 0; d < 5; d++) __NOP();  // ~500 ns delay
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);    // Read enable
    for (volatile int d = 0; d < 5; d++) __NOP();  // ~500 ns delay
    uint8_t byte = GPIOB->IDR & 0xFF;     // Read input
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);    // Read disable
    return byte;
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

void bank_switch(uint16_t bank, uint8_t mode){    // This was written for MBC5 SHOULD still work for other types.
    // Switch bank with proper timing
    switch(mode){
        case 1: // MBC1 Switching
            GBC_write_addr(0x2000);
            GBC_write_data(bank & 0x1F);        // Write lower 5 bits
            GBC_write_addr(0x2000);
            GBC_write_addr((bank >> 5) & 0x3);  // Write upper 2 bits
            break;

        case 2: // MBC2 Switching
            GBC_write_addr(0x0100);     // Bit 8 set for ROM banking
            GBC_write_data(bank & 0xF);  // Write lower 4 bits
            break;

        case 3:
            GBC_write_addr(0x2000);
            GBC_write_data(bank & 0x7F);        // Write all 7 bits
            break;

        case 5: // MBC5 Switching
            GBC_write_addr(0x3000);       // Bit 8
            GBC_write_data((bank >> 8) & 0x01);
            HAL_Delay(1);
            GBC_write_addr(0x2000);
            GBC_write_data(bank);
            GBC_set_data_input();          //  <‑‑ release PB0‑PB7
            HAL_Delay(1);  // Extended delay after bank switch (1ms)
            break;
        
        case 6: // MBC6 Switching
            GBC_write_addr(0x2800); // Set ROM mode
            GBC_write_data(0x00);   
            GBC_write_addr(0x2000); // Select bank (8KiB)
            GBC_write_data(bank);
            break;

        case 7:
            GBC_write_addr(0x2000); // Select bank
            GBC_write_data(bank);

        defualt:    // Weird MBC types (MMM01, M161, HuC1, HuC-3, etc.) tbd
            break;  
    }
}

void GBC_dump_cart(void){
    // === DEBUG OUTPUT ===
    // printf("\r\nCatridge Title: ");
    // for(int addr = 0x134; addr < 0x144; addr++){
    //     int byte = GBC_read(addr);
    //     printf("%c", byte);
    // }

    // printf("\r\nMemory Banking Scheme: ");
    uint32_t addr = 0x0147;
    uint8_t byte = GBC_read(addr);
    switch(byte){   // Determine MBC type, dump accordingly
        case 0x00:
        case 0x08:
        case 0x09:
            // printf("\n\rMBC0");
            dump_MBC0();
            break;

        case 0x01:
        case 0x02:
        case 0x03:
            // printf("\n\rMBC1");
            dump_MBC1();
            break;

        case 0x05:
        case 0x06:
            // printf("\n\rMBC2");
            dump_MBC1();    // Handled as MBC1 for now
            break;

        case 0x0B:
        case 0x0C:
        case 0x0D:
            // printf("\n\rMMM01 Not yet supported");
            break;

        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            // printf("\n\rMBC3");
            break;

        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
            // printf("\n\rMBC5");
            dump_MBC5();
            break;

        case 0x20:
            // printf("\n\rMBC6");
            break;

        case 0x22:
            // printf("\n\rMBC7");
            break;
        
        default:
            // printf("\n\rERROR Unsupported MBC");
            break;
    }
}

// MBC0 (No MBC) 32KiB Only
void dump_MBC0(void){ 
    uint8_t byte = 0x00;
    for(uint16_t addr = 0x0000; addr < 0x8000; addr++){
        byte = GBC_read(addr);
        HAL_UART_Transmit(&huart1, &byte, 1, HAL_MAX_DELAY);
    }
}

// MBC1 2MiB Max
void dump_MBC1(void){

    uint8_t byte = GBC_read(0x0148);    // Read rom size
    uint32_t banks = 2 << byte;         // Convert "size" to banks

    for(uint16_t addr = 0x0000; addr < 0x8000; addr++){ // Dump first two banks
        byte = GBC_read(addr);
        HAL_UART_Transmit(&huart1, &byte, 1, HAL_MAX_DELAY);
    }

    if(banks > 2){
        for(int bank = 2; bank < banks; bank++){    // Dump subsequent banks
            bank_switch(bank, 1);
            for(int addr = 0x4000; addr < 0x8000; addr++){  // Read new bank
                byte = GBC_read(addr);
                HAL_UART_Transmit(&huart1, &byte, 1, HAL_MAX_DELAY);
                // printf("%02X", byte); 
            }
        }
    }
}

// MBC2 up to 256Kib
void dump_MBC2(void){ // Handled as MBC1

    uint8_t byte = GBC_read(0x0148);    // Read rom size
    uint32_t banks = 2 << byte;         // Convert "size" to banks

    for(uint16_t addr = 0x0000; addr < 0x8000; addr++){ // Dump first two banks
        byte = GBC_read(addr);
        HAL_UART_Transmit(&huart1, &byte, 1, HAL_MAX_DELAY);
    }

    if(banks > 2){
        for(int bank = 2; bank < banks; bank++){    // Dump subsequent banks
            bank_switch(bank, 2);
            for(int addr = 0x4000; addr < 0x8000; addr++){  // Read new bank
                byte = GBC_read(addr);
                HAL_UART_Transmit(&huart1, &byte, 1, HAL_MAX_DELAY);
                // printf("%02X", byte); 
            }
        }
    }
}

// MBC3 up to 2MiB - Essentially the same as MBC1 and MBC2 other than switching and RAM
void dump_MBC3(void){
    uint8_t byte = GBC_read(0x0148);
    uint32_t banks = 2 << byte;

    for(uint16_t addr = 0x0000; addr < 0x8000; addr++){ // Dump first two banks
        byte = GBC_read(addr);
        HAL_UART_Transmit(&huart1, &byte, 1, HAL_MAX_DELAY);
    }

    if(banks > 2){
        for(int bank = 2; bank < banks; bank++){    // Dump subsequent banks
            bank_switch(bank, 3);
            for(int addr = 0x4000; addr < 0x8000; addr++){  // Read new bank
                byte = GBC_read(addr);
                HAL_UART_Transmit(&huart1, &byte, 1, HAL_MAX_DELAY);
                // printf("%02X", byte); 
            }
        }
    }
}

// MBC5 up to 8MiB
void dump_MBC5(void){
    uint8_t byte = GBC_read(0x0148);    // Read rom size
    uint32_t banks = 2 << byte;         // Convert "size" to banks
    // printf("\r\nROM Size: %d Kilobytes", banks * 16);

    // printf("\r\n----BEGIN ROM----\r\n");    // Prepare to print rom

    for(uint16_t i = 0; i < banks; i++){
        bank_switch(i, 5);
        for(int addr = 0x4000; addr < 0x8000; addr++){
            byte = GBC_read(addr);
            HAL_UART_Transmit(&huart1, &byte, 1, HAL_MAX_DELAY);
            // printf("%02X", byte); 
        }
    }
}