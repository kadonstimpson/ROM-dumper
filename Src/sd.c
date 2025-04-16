#include "sd.h"
//use fatfs for filesystem, requires implementation of hardware accesses.

// Put code to run here
void sd_test(){

}

// sd card connect to spi2
void init_spi_sd()
{
  //enable clocks
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

  //setup gpio for spi2
  //pb12 NSS
  //pb13 SCK
  //pb14 MISO
  //pb15 MOSI
  // all use AF0
  GPIO_InitTypeDef initSPI2 = {
    .Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
    .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_NOPULL,
    .Speed = GPIO_SPEED_FREQ_LOW,
    .Alternate = GPIO_AF0_SPI2
  };
  HAL_GPIO_Init(GPIOC, &initSPI2);

  //configure spi2
	SPI2->CR1 &= ~SPI_CR1_SPE; //make sure disabled

  //set baud rate to minimum
	SPI2->CR1 &= ~SPI_CR1_BR; 
	SPI2->CR1 |=  SPI_CR1_BR_0;
	SPI2->CR1 |=  SPI_CR1_BR_1;
	SPI2->CR1 |=  SPI_CR1_BR_2;

  //master mode, force NSS to 1
	SPI2->CR1 |=  SPI_CR1_MSTR;
	SPI2->CR1 |=  SPI_CR1_SSM;
	SPI2->CR1 |=  SPI_CR1_SSI;

  //8 bit data size
	SPI2->CR2 &= ~SPI_CR2_DS;
	SPI2->CR2 |=  (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2 | SPI_CR2_FRXTH);

  //enable spi
	SPI2->CR1 |=  SPI_CR1_SPE;
}

void init_sd()
{

}