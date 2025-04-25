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
FIL gba_file;               // file pointer
uint8_t gba_buf[512];          // write buffer
UINT gba_bw = 0;                   // write result
uint16_t gba_buf_index = 0;        // index into buffer
extern FATFS fs;

void GBA_test(void){
  GBA_dump_cart();
  // #define CHUNK_BYTES     64                      // 32 half‑words  (=64 bytes)
  // #define MAX_ROM_SIZE    (32 * 1024 * 1024)      // 32 MiB max
  // #define BLANK_1         0xFFFF
  // #define BLANK_2         0x0000

  // uint8_t raw[CHUNK_BYTES];
  // uint8_t k = 0;

  // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);   // enable /CS
  // __NOP(); __NOP();
  // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);   // enable /CS
  // __NOP(); __NOP();
  
  // uint32_t rom_size = 0x800000;         // 8 MiB GBA title

  // for (uint32_t addr = 0; addr < (rom_size >> 1); addr++) {
  //   uint16_t w = GBA_read_addr(addr);  // read half‑word
  //   w = (w << 8) | (w >> 8);                // byte‑swap

  //   raw[k++] = w >> 8;      // Write low byte
  //   raw[k++] = w & 0xFF;    // Write high byte

  //   if (k == CHUNK_BYTES) {
  //     while (CDC_Transmit_FS(raw, CHUNK_BYTES) == USBD_BUSY);
  //     k = 0;
  //   }
  // }
  
  // // last partial chunk
  // if (k){
  //   while (CDC_Transmit_FS(raw, k) == USBD_BUSY);
  // }
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

void GBA_dump_cart(void) {
  FRESULT res;
  // char title[17] = {0};         // room for 16 + null
  char filename[32] = "dump.gba";      // output name: TITLE.gba
  const uint32_t max_words = 0x2000000 >> 1; // 16M half-words = 32 MiB
  const uint32_t progress_step = (512 * 1024) >> 1; // Half-words per 512 KiB


  // === 1. Read GBA title from 0xA0–0xAB ===
  // Extract title from ROM address 0xA0–0xAB (12 chars)
  // char raw_title[17] = {0};
  // for (int i = 0; i < 12; i++) 
  // {
  //   uint16_t w = GBA_read_addr((0xA0 >> 1) + i);
  //   raw_title[i * 2 + 0] = w & 0xFF;
  //   raw_title[i * 2 + 1] = w >> 8;
  // }

  // // Sanitize title into safe filename characters
  // char title[17] = {0};
  // for (int i = 0; i < 16; i++) 
  // {
  //   char c = raw_title[i];
  //   if (c == '\0') break;
  //   if (c == ' ') c = '_';
  //   else if (!(c >= '0' && c <= '9') &&
  //             !(c >= 'A' && c <= 'Z') &&
  //             !(c >= 'a' && c <= 'z') &&
  //             c != '_') 
  //   {
  //     c = '_'; // replace invalid chars
  //   }
  //   title[i] = c;
  // }

  // // Use fallback name if sanitized title is empty
  // char filename[32];
  // if (title[0] == '\0') 
  // {
  //   strcpy(filename, "dump.gba");
  // } 
  // else 
  // {
  //   snprintf(filename, sizeof(filename), "%.16s.gba", title);
  // }

  // printf("ROM Title: %s\n\r", title);
  // printf("Dumping to file: %s\n\r", filename);


  // === 2. Auto-detect ROM size ===
  uint32_t last_non_blank = 0;
  for (uint32_t addr = max_words - 1; addr > 0; addr--) 
  {
    uint16_t w = GBA_read_addr(addr);
    if (w != 0xFFFF && w != 0x0000) 
    {
      last_non_blank = addr;
      break;
    }
  }
  uint32_t rom_size = (last_non_blank + 1) * 2;
  // uint32_t rom_size = 0x800000;         // 8 MiB GBA title
  printf("Detected ROM size: %lu bytes\n\r", rom_size);

  // === 3. Open file for write ===
  res = f_open(&gba_file, filename, FA_WRITE | FA_CREATE_ALWAYS);
  if (res != FR_OK) 
  {
    printf("f_open result = %d\r\n", res);
    return;
  }

  // === 4. Dump ROM ===
  gba_buf_index = 0;
  for (uint32_t addr = 0; addr <= last_non_blank; addr++) 
  {
    uint16_t w = GBA_read_addr(addr);
    w = (w << 8) | (w >> 8);  // byte-swap

    gba_buf[gba_buf_index++] = w >> 8;
    gba_buf[gba_buf_index++] = w & 0xFF;

    if (gba_buf_index == 512) 
    {
      res = f_write(&gba_file, gba_buf, 512, &gba_bw);
      if (res != FR_OK || gba_bw != 512) 
      {
        printf("f_write error = %d, wrote %u bytes\r\n", res, gba_bw);
        f_close(&gba_file);
        return;
      }
      gba_buf_index = 0;
    }

    if ((addr % progress_step) == 0) 
    {
      printf("Progress: %lu / %lu KiB\n\r", (addr * 2) >> 10, rom_size >> 10);
    }
  }

  // Flush remaining
  if (gba_buf_index > 0) 
  {
    res = f_write(&gba_file, gba_buf, gba_buf_index, &gba_bw);
    if (res != FR_OK || gba_bw != gba_buf_index) 
    {
      printf("f_write (tail) error = %d, wrote %u bytes\r\n", res, gba_bw);
    }
  }

  f_close(&gba_file);
  printf("Dump complete: %lu bytes saved to %s\n\r", rom_size, filename);
}

