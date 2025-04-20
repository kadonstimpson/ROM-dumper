#include "GBA.h"

// #define DATA_PORT GPIOC
// #define ADDR_LOW_PORT GPIOC   // PC0-PC15
// #define ADDR_HIGH_PORT GPIOB  // PB0-PB7
// #define CART_RD_PORT GPIOB
// #define CART_RD_PIN GPIO_PIN_10

static uint8_t GBA_bus_mode = 0xFF;  // 0 = data input, 1 =  data output, 2 = address output 0xFF = uninitialized

void GBA_test(void){
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);   // enable /CS
    __NOP(); __NOP();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);   // enable /CS
    __NOP(); __NOP();

    for(uint32_t addr = 0x000000; addr < 0x800000; addr++){
        uint16_t word = GBA_read_addr(addr);
        word = (word << 8) | ((word >> 8) & 0xFF); // Bytes swap
        printf("%04X", word);
    }

}

void GBA_set_data_output(void) {
    if (GBA_bus_mode != 1) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GBA_DAT;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        GBA_bus_mode = 1;
    }
}

void GBA_set_data_input(void) {
    if (GBA_bus_mode != 0) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GBA_DAT;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        GBA_bus_mode = 0;
    }
}

void GBA_set_address_output(void) {
    if (GBA_bus_mode != 2) {
        GPIO_InitTypeDef GPIO_InitStruct_L = {0}; // Set address low bits (PC0-PC15) output
        GPIO_InitStruct_L.Pin = GBA_ADDR_L;
        GPIO_InitStruct_L.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct_L.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct_L);

        GPIO_InitTypeDef GPIO_InitStruct_H = {0}; // Set address high bits (PB0-PB7) output
        GPIO_InitStruct_H.Pin = GBA_ADDR_H;
        GPIO_InitStruct_H.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct_H.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct_H);
        GBA_bus_mode = 2;
    }
}

void GBA_write_addr(uint32_t addr){
    GBA_set_address_output();
    GPIOC->ODR = (addr & 0xFFFF);
    uint32_t high_bits = (addr >> 16) & 0xFF;
    GPIOB->BSRR = ((~high_bits & 0xFF) << 16) | high_bits;
}

uint16_t GBA_read_addr(uint32_t addr){
    GBA_write_addr(addr);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);   // enable /CS
    __NOP(); __NOP();
    GBA_set_data_input();
    for (volatile int d = 0; d < 5; d++) __NOP();  // delay
    
    
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);    // Read enable
    for (volatile int d = 0; d < 10; d++) __NOP();  // delay
    uint16_t word = GPIOC->IDR;     // Read input
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);    // Read disable
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);   // disable /CS
    return word;
}

// --- Eric's Functions Below ---

// void set_address(uint16_t addr) {
//     // 写入 PC0-PC15
//     for (int i = 0; i < 16; i++) {
//         HAL_GPIO_WritePin(GPIOC, (1 << i), (addr & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
//     }
// }

// uint8_t read_data_byte(void) {
//     uint8_t value = 0;
//     for (int i = 0; i < 8; i++) {
//         value |= (HAL_GPIO_ReadPin(DATA_PORT, (1 << i)) << i);
//     }
//     return value;
// }

// --- Cubby's Test Functions Below ---

// void GBA_write_addr(uint32_t addr){

//     __HAL_RCC_GPIOB_CLK_ENABLE();   // Enable GPIOB clock
//     __HAL_RCC_GPIOC_CLK_ENABLE();   // Enable GPIOC clock

//     GPIO_InitTypeDef ADDR_H = {0};  // Upper address bus
//     ADDR_H.Pin = GBA_ADDR_H;
//     ADDR_H.Mode = GPIO_MODE_OUTPUT_PP;  // Push-pull output
//     ADDR_H.Pull = GPIO_NOPULL;
//     ADDR_H.Speed = GPIO_SPEED_FREQ_LOW;

//     GPIO_InitTypeDef ADDR_L = {0};  // Lower address bus
//     ADDR_L.Pin = GBA_ADDR_L;
//     ADDR_L.Mode = GPIO_MODE_OUTPUT_PP;  // Push-pull output
//     ADDR_L.Pull = GPIO_NOPULL;
//     ADDR_L.Speed = GPIO_SPEED_FREQ_LOW;

//     HAL_GPIO_Init(GPIOC, &ADDR_H);
//     HAL_GPIO_Init(GPIOB, &ADDR_L);

//     GPIOC->ODR = (addr >> 8) & 0xFFFF;
//     GPIOB->ODR = (addr >> 24) & 0x00FF;

// }

// uint32_t GBA_read(void){
//     GPIO_InitTypeDef GPIO_InitStruct = {0};

//     GPIO_InitStruct.Pin = GPIO_PIN_All;
//     GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//     GPIO_InitStruct.Pull = GPIO_NOPULL;

//     HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

//     uint32_t data = GPIOC->IDR & 0xFFFF;     // Read input

//     return(data);
// }