#include "stm32f0xx_hal.h"
#include <string.h>
#include <stdio.h>
#include "GBC.h"
#include "GBA.h"
#include "sd.h"
#include "main.h"
#include "uart.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

volatile uint32_t last_isr;

void Init_All(void);
static void Gyro_SPI2_Init();
static void USB_ClockEnable();

FATFS fs;

int main(void) {
  Init_All();   // Add any additional initialization here

  // initialize sd card
  // disk_initialize(0);
  // try to mount sd card
  FRESULT res = f_mount(&fs, "", 1);

  if (res != FR_OK)
  {
    printf("f_mount result = %d\r\n", res);
  }

  // GBA_test();   // Test GBA stuff here

  // GBC_test();
  GBA_test();
  // sd_cmd_test();
  // sd_test();

  f_unmount("");
  // sd_test();

  while(1){
    CDC_Transmit_FS((uint8_t *)"Hello, host!\\r\\n", 14);
  }
}

void Init_All(){
  HAL_Init();
  SystemClock_Config();
  USB_ClockEnable();

  __HAL_RCC_GPIOA_CLK_ENABLE();  // Enable GPIOA clock
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  __HAL_RCC_PWR_CLK_ENABLE();   // Enable USB clock and power
  MX_USB_DEVICE_Init();      // Hangs, removed temporarily
 
  HAL_PWR_EnableBkUpAccess(); // Enable access to backup domain

  RCC->BDCR &= ~RCC_BDCR_LSEON; // Disable LSE oscillator to free PC14 and PC15

  while (RCC->BDCR & RCC_BDCR_LSERDY) {}  // Wait until LSE is fully disabled

  Gyro_SPI2_Init();
  GPIO_UART1_Init();  
  UART1_Init(); 
  MX_RTC_Init();
  rtc_check_and_set();

  setvbuf(stdout, NULL, _IONBF, 0); // make printf unbuffered 

  GPIO_InitTypeDef RDWR = {0};  // Upper address bus
  RDWR.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
  RDWR.Mode = GPIO_MODE_OUTPUT_PP;  // Push-pull output
  RDWR.Pull = GPIO_NOPULL;
  RDWR.Speed = GPIO_SPEED_FREQ_LOW;

  GPIO_InitTypeDef RST = {0};  // Cart Reset
  RST.Pin = GPIO_PIN_0;
  RST.Mode = GPIO_MODE_OUTPUT_PP;  // Push-pull output
  RST.Pull = GPIO_NOPULL;
  RST.Speed = GPIO_SPEED_FREQ_LOW;
  
  HAL_GPIO_Init(GPIOB, &RDWR);
  HAL_GPIO_Init(GPIOA, &RST);

  shift_enable();

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);      // Read disable
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);       // Write disable
  // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);    // Pull /CS low

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);     // Reset cart
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
  HAL_Delay(10);
}

static void Gyro_SPI2_Init(void)  // set gyro pins to high impedence
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    g.Pin       = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;  // PB13 - SCK, PB14 - MISO, PB15 - MOSI
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF0_SPI2;
    HAL_GPIO_Init(GPIOB, &g);

    __HAL_RCC_GPIOC_CLK_ENABLE(); // PC0 - CS
    g.Pin   = GPIO_PIN_0;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &g);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);  // CS high

    static SPI_HandleTypeDef h;
    h.Instance               = SPI2;
    h.Init.Mode              = SPI_MODE_MASTER;
    h.Init.Direction         = SPI_DIRECTION_2LINES;
    h.Init.DataSize          = SPI_DATASIZE_8BIT;
    h.Init.CLKPolarity       = SPI_POLARITY_LOW;
    h.Init.CLKPhase          = SPI_PHASE_1EDGE;
    h.Init.NSS               = SPI_NSS_SOFT;
    h.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    h.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    HAL_SPI_Init(&h);

    uint8_t wr[2];

    /* 1. Power‑down the device (CTRL_REG1, addr 0x20) */
    wr[0] = 0x20;               // address, MSB=0 → write
    wr[1] = 0x00;               // PD=0, ODR=00 => power‑down
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&h, wr, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);

    /* 2. Disable every interrupt on both pins (CTRL_REG3, addr 0x22) */
    wr[0] = 0x22;
    wr[1] = 0x10;               // all I2_/I1_ high impedence
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&h, wr, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
}

static void USB_ClockEnable(void){
  __HAL_RCC_HSI48_ENABLE();
  while(!__HAL_RCC_GET_FLAG(RCC_FLAG_HSI48RDY));

  __HAL_RCC_USB_CONFIG(RCC_USBCLKSOURCE_HSI48);
  __HAL_RCC_CRS_CLK_ENABLE();
  __HAL_RCC_USB_CLK_ENABLE();
}
