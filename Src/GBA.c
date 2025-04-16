// #define DATA_PORT GPIOC
// #define ADDR_LOW_PORT GPIOC   // PC0-PC15
// #define ADDR_HIGH_PORT GPIOB  // PB0-PB7
// #define CART_RD_PORT GPIOB
// #define CART_RD_PIN GPIO_PIN_10

void GBA_test(void){

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