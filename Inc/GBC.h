#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"
#include "ff.h"

#define GBC_ADDR            GPIO_PIN_All                            // P0 - P15 (GPIOC) 
#define GBC_DAT             (GPIO_PIN_All & ((uint16_t)0x00FF))    // P0 - P7  (GPIOB)

void GBC_test(void);
void shift_enable(void);
void GBA_write_addr(uint32_t addr);
void GBC_write_addr(uint32_t addr);
uint32_t GBA_read(void);
uint8_t GBC_read(uint32_t addr);
void GBC_write_data(uint8_t byte);
void GBC_set_data_output(void);
void GBC_set_data_input(void);
void bank_switch(uint16_t bank, uint8_t mode);
void GBC_dump_cart(void);
void dump_MBC0(void);
void dump_MBC1(void);   // 1 - 3 ROM dumping largely the same. Could be joined
void dump_MBC2(void);   // but saves are handled very differently, so leaving
void dump_MBC3(void);   // separate for now.
void dump_MBC5(void);
void dump_MBC6(void);
void dump_MCB7(void);