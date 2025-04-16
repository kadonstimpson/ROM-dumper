#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"

#define GBA_ADDR_H          GPIO_PIN_All                            // P0 - P15 (GPIOC)
#define GBA_ADDR_L          (GPIO_PIN_All & ((uint16_t)0x00FF))    // P0 - P7  (GPIOB)
#define GBA_DAT             GPIO_PIN_All                            // P0 - P15 (GPIOC)
#define GBC_ADDR            GPIO_PIN_All                            // P0 - P15 (GPIOC) 
#define GBC_DAT             (GPIO_PIN_All & ((uint16_t)0x00FF))    // P0 - P7  (GPIOB)

void GBC_test(void);
void shift_enable(void);
void GBA_write_addr(uint32_t addr);
void GBC_write_addr(uint32_t addr);
uint32_t GBA_read(void);
uint8_t GBC_read(void);
void GBC_read_bank(uint32_t start_addr);
void GB_write_data(uint8_t byte);
void GBC_set_data_output(void);
void GBC_set_data_input(void);