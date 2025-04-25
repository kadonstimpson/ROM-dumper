#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"

#define GBA_ADDR_L          GPIO_PIN_All                            // P0 - P15 (GPIOC)
#define GBA_ADDR_H          (GPIO_PIN_All & ((uint16_t)0x00FF))     // P0 - P7  (GPIOB)
#define GBA_DAT             GPIO_PIN_All                            // P0 - P15 (GPIOC)

void GBA_test(void);
void GBA_set_data_output(void);
void GBA_set_data_input(void);
void GBA_set_address_output(void);
void GBA_write_addr(uint32_t addr);
uint16_t GBA_read_addr(uint32_t addr);
void GBA_dump_cart(void);