#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"
#include "GBC.h"
#include "uart.h"
#include <stdio.h>

static uint8_t data_bus_mode = 0xFF;  // 0 = input, 1 = output, 0xFF = uninitialized
FIL gbc_file;
volatile uint8_t gbc_buf[512]; //global write buffer
volatile uint16_t gbc_buf_index = 0; //buffer index
volatile UINT gbc_bw = 0;
extern FATFS fs;



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

    default:    // Weird MBC types (MMM01, M161, HuC1, HuC-3, etc.) tbd
      break;  
  }
}

void GBC_dump_cart(void){
  // === DEBUG OUTPUT ===
  // printf("\r\nCatridge Title: ");
  char title[17];
  int i = 0;

  // read title
  for(int addr = 0x134; addr < 0x144; addr++){
    title[i] = GBC_read(addr);
    i++;
  }
  title[16] = '\0'; //add null terminator at end

  //file variables
  char filename[32];
  FRESULT res;

  sprintf(&filename, "%s.gbc", title);
  printf("filename = %s\n\r", filename);

  // Try to open file for writing
  res = f_open(&gbc_file, filename, FA_WRITE | FA_CREATE_ALWAYS);
  // res = f_open(&gbc_file, "cart.gbc", FA_WRITE | FA_CREATE_ALWAYS);
  if (res != FR_OK)
  {
    printf("f_open result = %d\r\n", res);
    return;
  }

  // printf("\r\nMemory Banking Scheme: ");
  uint32_t addr = 0x0147;
  uint8_t byte = GBC_read(addr);
  switch(byte){   // Determine MBC type, dump accordingly
    case 0x00:
    case 0x08:
    case 0x09:
      // printf("\n\rMBC0");
      GBC_dump_MBC(0);
      break;

    case 0x01:
    case 0x02:
    case 0x03:
      // printf("\n\rMBC1");
      GBC_dump_MBC(1);
      break;

    case 0x05:
    case 0x06:
      // printf("\n\rMBC2");
      GBC_dump_MBC(2);
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
      GBC_dump_MBC(3);
      break;

    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
      // printf("\n\rMBC5");
    //   GBC_dump_MBC(5);
        dump_MBC5();
      break;

    case 0x20:
      // printf("\n\rMBC6");
      GBC_dump_MBC(6);
      break;

    case 0x22:
      // printf("\n\rMBC7");
      GBC_dump_MBC(7);
      break;
    
    default:
      // printf("\n\rERROR Unsupported MBC");
      break;
  }
  printf("Successfully dumped %s to %s\n\r", title, filename);
  f_close(&gbc_file);
}

void GBC_dump_MBC(uint8_t mbc_type) {
  uint8_t rom_size = GBC_read(0x0148);
  uint32_t banks = 2 << rom_size;
  gbc_buf_index = 0;

  // Handle MBC6 special case
  if (mbc_type == 6)
    banks *= 2;

  for (uint16_t i = (mbc_type == 0 ? 0 : 1); i < banks; i++) {
    bank_switch(i, mbc_type);
    gbc_buf_index = 0;

    // Most MBCs have 0x4000 - 0x7FFF
    uint16_t start = 0x4000;
    uint16_t end = 0x8000;

    if (mbc_type == 0) {
      start = 0x0000;
      end = 0x8000;
    } else if (mbc_type == 6) {
      end = 0x6000; // MBC6 is 0x4000–0x5FFF
    }

    for (uint32_t addr = start; addr < end; addr++) {
      gbc_buf[gbc_buf_index++] = GBC_read(addr);
      if (gbc_buf_index == 512) {
        FRESULT res = f_write(&gbc_file, gbc_buf, 512, &gbc_bw);
        if (res != FR_OK || gbc_bw != 512) {
          printf("f_write error = %d, wrote %u bytes\r\n", res, gbc_bw);
          return;
        }
        gbc_buf_index = 0;
      }
    }

    // Write any remaining data
    if (gbc_buf_index > 0) {
      FRESULT res = f_write(&gbc_file, gbc_buf, gbc_buf_index, &gbc_bw);
      if (res != FR_OK || gbc_bw != gbc_buf_index) {
        printf("f_write (tail) error = %d, wrote %u bytes\r\n", res, gbc_bw);
        return;
      }
    }
  }
}

void dump_MBC5(void) 
{
  uint8_t rom_size = GBC_read(0x0148);    // Read ROM size
  uint32_t banks = 2 << rom_size;         // Convert size code to # of banks
  uint8_t buffer[512];
  UINT bw;
  const uint32_t progress_step = (512 * 1024) >> 1; // Half-words per 512 KiB

  for (uint16_t i = 0; i < banks; i++) {
    bank_switch(i, 5);
    uint16_t buf_index = 0;

    for (int addr = 0x4000; addr < 0x8000; addr++) {
      buffer[buf_index++] = GBC_read(addr);

      if (buf_index == 512) 
      {
        FRESULT res = f_write(&gbc_file, buffer, 512, &bw);
        if (res != FR_OK || bw != 512) 
        {
          printf("f_write error = %d, wrote %u bytes\r\n", res, bw);
          return;
        }
        buf_index = 0;
      }
    }

    // Write remaining (if any)
    if (buf_index > 0) 
    {
      FRESULT res = f_write(&gbc_file, buffer, buf_index, &bw);
      if (res != FR_OK || bw != buf_index) 
      {
        printf("f_write (tail) error = %d, wrote %u bytes\r\n", res, bw);
        return;
      }
    }
  }
}