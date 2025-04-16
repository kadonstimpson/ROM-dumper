#pragma once
#include "stm32f0xx.h"
#include <stm32f072xb.h>
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_spi.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_gpio_ex.h"

void sd_test(void);
void init_spi_sd(void);

void init_sd(void);