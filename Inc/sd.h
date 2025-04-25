#pragma once
#include "stm32f0xx.h"
#include <stm32f072xb.h>
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_spi.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_gpio_ex.h"
#include "diskio.h"
#include "ff.h"

#include <stdio.h>
#include "uart.h"

// Test function
void sd_test(void);
void sd_cmd_test();

// Hardware initialization
void init_spi_sd(void);
DSTATUS init_sd(void);
void set_spi_speed_fast(void);

// Chip select control
void sd_cs_low(void);
void sd_cs_high(void);

// SPI utilities
uint8_t sd_spi_send(uint8_t data);
uint8_t sd_spi_recv(void);

// SD command interface
uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc);

// FatFs low-level glue functions (used in diskio.c)
DSTATUS sd_status(void);
DRESULT sd_read(BYTE *buff, LBA_t sector, UINT count);
DRESULT sd_write(const BYTE *buff, LBA_t sector, UINT count);
// DRESULT sd_ioctl(BYTE cmd, void *buff);