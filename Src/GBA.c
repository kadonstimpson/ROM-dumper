#include "GBA.h"
#include "uart.h"
#include "usbd_cdc_if.h"
#include "ff.h"

// #define DATA_PORT GPIOC
// #define ADDR_LOW_PORT GPIOC   // PC0-PC15
// #define ADDR_HIGH_PORT GPIOB  // PB0-PB7
// #define CART_RD_PORT GPIOB
// #define CART_RD_PIN GPIO_PIN_10

static uint8_t GBA_bus_mode = 0xFF;  // 0 = data input, 1 =  data output, 2 = address output 0xFF = uninitialized

void GBA_test(void){
  #define CHUNK_BYTES     64                      // 32 half‑words  (=64 bytes)
  #define MAX_ROM_SIZE    (32 * 1024 * 1024)      // 32 MiB max
  #define BLANK_1         0xFFFF
  #define BLANK_2         0x0000

  uint8_t raw[CHUNK_BYTES];
  uint8_t k = 0;

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);   // enable /CS
  __NOP(); __NOP();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);   // enable /CS
  __NOP(); __NOP();
  
  uint32_t rom_size = 0x800000;         // 8 MiB GBA title

  for (uint32_t addr = 0; addr < (rom_size >> 1); addr++) {
    uint16_t w = GBA_read_addr(addr);  // read half‑word
    w = (w << 8) | (w >> 8);                // byte‑swap

    raw[k++] = w >> 8;      // Write low byte
    raw[k++] = w & 0xFF;    // Write high byte

    if (k == CHUNK_BYTES) {
        while (CDC_Transmit_FS(raw, CHUNK_BYTES) == USBD_BUSY);
        k = 0;
    }
  }
  
  // last partial chunk
  if (k){
    while (CDC_Transmit_FS(raw, k) == USBD_BUSY);
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
  // __NOP(); __NOP();
  GBA_set_data_input();
  // for (volatile int d = 0; d < 5; d++) __NOP();  // delay
  
  
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);    // Read enable
  for (volatile int d = 0; d < 4; d++) __NOP();  // delay
  uint16_t word = GPIOC->IDR;     // Read input
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);    // Read disable
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);   // disable /CS
  return word;
}