#include "sd.h"
#include "diskio.h"
#include "ff.h"
#include "rtc.h"

static DSTATUS sd_stat = STA_NOINIT;
extern RTC_HandleTypeDef hrtc;
static uint8_t is_sdhc = 0;

void sd_cs_low(void)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
}

void sd_cs_high(void)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}

void sd_cmd_test()
{
  uint8_t buffer[512];
  // uint32_t addr = 0;  // Sector 0 (valid for both SDHC and SDSC if logic is correct)
  uint32_t addr = 40960;
  printf("=== SD Sector 0 Read Test ===\n");

  // Send CMD17 to read block 0
  uint8_t r1 = sd_send_cmd(17, addr, 0xFF);
  printf("CMD17 response: 0x%02X\n", r1);

  if (r1 != 0x00) {
    printf("CMD17 failed! (R1 = 0x%02X)\n", r1);
    sd_cs_high();
    sd_spi_recv();
    return;
  }

  // Wait for data token 0xFE
  int timeout = 100000;
  uint8_t token;
  do {
    token = sd_spi_recv();
  } while (token == 0xFF && --timeout);

  if (token != 0xFE) {
    printf("Data token not received: 0x%02X\n", token);
    sd_cs_high();
    sd_spi_recv();
    return;
  }

  printf("Data token received: 0x%02X\n", token);

  // Read 512 bytes
  for (int i = 0; i < 512; i++) {
    buffer[i] = sd_spi_recv();
  }

  // Discard CRC
  sd_spi_recv();
  sd_spi_recv();

  sd_cs_high();
  sd_spi_recv();

  printf("Read complete. First 16 bytes:\n");
  for (int i = 0; i < 16; i++) {
    printf("%02X ", buffer[i]);
  }
  printf("\n=== Done ===\n");
}

void sd_test()
{
  // FATFS fs;
  FIL file;
  UINT bw, br;
  FRESULT res;
  char buffer[64];

  // Try to mount
  // res = f_mount(&fs, "", 1);
  // printf("f_mount result = %d\r\n", res);
  // if (res != FR_OK) return;

  // Try to open file for writing
  res = f_open(&file, "helloworld.txt", FA_WRITE | FA_CREATE_ALWAYS);
  printf("f_open result = %d\r\n", res);
  if (res != FR_OK) return;

  // Write to file
  res = f_write(&file, "Hello, STM32 SD write!\n", 23, &bw);
  printf("f_write result = %d, bytes = %u\r\n", res, bw);
  f_close(&file);

  // Try to read it back
  res = f_open(&file, "helloworld.txt", FA_READ);
  printf("f_open (read) = %d\r\n", res);
  if (res != FR_OK) return;

  res = f_read(&file, buffer, sizeof(buffer) - 1, &br);
  buffer[br] = 0;
  printf("f_read = %d, read %u bytes: %s\r\n", res, br, buffer);
  f_close(&file);
}


void init_spi_sd()
{
  // Enable GPIOB, SPI2
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
  RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

  // PB13-SCK, PB14-MISO, PB15-MOSI: AF0
  GPIO_InitTypeDef initSPI2 = {
    .Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
    .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_NOPULL,
    .Speed = GPIO_SPEED_FREQ_LOW,
    .Alternate = GPIO_AF0_SPI2
  };
  HAL_GPIO_Init(GPIOB, &initSPI2);

  // PB12-CS: Output
  GPIO_InitTypeDef initCS = {
    .Pin = GPIO_PIN_12,
    .Mode = GPIO_MODE_OUTPUT_PP,
    .Pull = GPIO_NOPULL,
    .Speed = GPIO_SPEED_FREQ_LOW
  };
  HAL_GPIO_Init(GPIOB, &initCS);
  sd_cs_high(); // Default CS high

  // SPI2 configuration
  SPI2->CR1 &= ~SPI_CR1_SPE; // disable SPI
  SPI2->CR1 &= ~SPI_CR1_BR;
  SPI2->CR1 |=  SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2; // slowest

  SPI2->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI; // master, SW NSS
  SPI2->CR2 &= ~SPI_CR2_DS;
  SPI2->CR2 |= SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2 | SPI_CR2_FRXTH; // 8-bit

  SPI2->CR1 |= SPI_CR1_SPE; // enable SPI
}

void set_spi_speed_fast(void) 
{
  SPI2->CR1 &= ~SPI_CR1_SPE;
  SPI2->CR1 &= ~SPI_CR1_BR;
  SPI2->CR1 |= SPI_CR1_BR_1; // /8
  SPI2->CR1 |= SPI_CR1_SPE;
}


uint8_t sd_spi_send(uint8_t data)
{
  while (!(SPI2->SR & SPI_SR_TXE));
  *((__IO uint8_t*)&SPI2->DR) = data;
  while (!(SPI2->SR & SPI_SR_RXNE));
  return *((__IO uint8_t*)&SPI2->DR);
}

uint8_t sd_spi_recv()
{
  return sd_spi_send(0xFF);
}

// uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
// {
//   sd_cs_high();             // Deselect card
//   sd_spi_recv();            // Give it 8 extra clocks
//   sd_cs_low();              // Select card
//   sd_spi_recv();            // Another 8 clocks after CS low

//   // Send command packet
//   sd_spi_send(0x40 | cmd);
//   sd_spi_send((arg >> 24) & 0xFF);
//   sd_spi_send((arg >> 16) & 0xFF);
//   sd_spi_send((arg >> 8) & 0xFF);
//   sd_spi_send(arg & 0xFF);
//   sd_spi_send(crc);

//   // Wait for valid response (R1: MSB must be 0)
//   for (int i = 0; i < 10; i++) 
//   {
//     uint8_t r = sd_spi_recv();
//     if ((r & 0x80) == 0) return r;  // Valid response
//   }

//   return 0xFF;  // Timeout
// }

uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
  // Deselect and give 8 clocks (card idle)
  sd_cs_high();
  sd_spi_recv();

  // Short delay between commands (optional but helpful)
  for (volatile int d = 0; d < 1000; d++);  // crude delay

  // Select card and give 8 clocks after CS low
  sd_cs_low();
  sd_spi_recv();

  // Send command packet
  sd_spi_send(0x40 | cmd);                   // Command index
  sd_spi_send((arg >> 24) & 0xFF);           // Argument[31:24]
  sd_spi_send((arg >> 16) & 0xFF);           // Argument[23:16]
  sd_spi_send((arg >> 8) & 0xFF);            // Argument[15:8]
  sd_spi_send(arg & 0xFF);                   // Argument[7:0]
  sd_spi_send(crc);                          // Dummy CRC or valid (for CMD0, CMD8)

  // Wait for R1 response (bit 7 = 0)
  for (int i = 0; i < 10; i++) {
    uint8_t r = sd_spi_recv();
    if ((r & 0x80) == 0) return r;           // Valid response
  }

  return 0xFF;  // Timeout
}




DSTATUS init_sd()
{
  init_spi_sd();
  sd_cs_high();
  for (int i = 0; i < 10; i++) sd_spi_recv();

  // CMD0: Go to idle
  uint8_t r = sd_send_cmd(0, 0x00000000, 0x95);
  if (r != 0x01) 
  {
    printf("CMD0 error: %x\n\r", r);
    return sd_stat = STA_NOINIT;
  }

  // CMD8: Check voltage range
  r = sd_send_cmd(8, 0x000001AA, 0x87);
  if (r != 0x01) return sd_stat = STA_NOINIT;

  uint8_t r7[4];
  for (int i = 0; i < 4; i++) r7[i] = sd_spi_recv();
  if (r7[2] != 0x01 || r7[3] != 0xAA)
  {
    printf("CMD8 error: %x\n\r", r);
    return sd_stat = STA_NOINIT;
  }
  // ACMD41 loop
  for (int i = 0; i < 1000; i++) 
  {
    sd_send_cmd(55, 0, 0x65); // CMD55
    r = sd_send_cmd(41, 0x40000000, 0x77); // ACMD41 with HCS
    if (r == 0x00) break;
    HAL_Delay(1);
  }
  set_spi_speed_fast(); //init done, speed up bus
  if (r != 0x00) return sd_stat = STA_NOINIT;

  // CMD58: Read OCR
  r = sd_send_cmd(58, 0, 0xFD);
  if (r != 0x00)
  {
    printf("CMD58 error: %x\n\r", r);
    return sd_stat = STA_NOINIT;
  }

  // for (int i = 0; i < 4; i++) sd_spi_recv(); // discard OCR
  // read OCR
  uint32_t ocr = 0;
  for (int i = 0; i < 4; i++) 
  {
    ocr = (ocr << 8) | sd_spi_recv();
  }
  is_sdhc = (ocr & (1UL << 30)) ? 1 : 0;  // CCS bit: 1 = SDHC/SDXC


  sd_cs_high();
  sd_spi_recv();


  sd_stat = 0; // Ready
  return sd_stat;
}

// DRESULT sd_read(BYTE *buff, LBA_t sector, UINT count)
// {
//   if (sd_stat & STA_NOINIT) return RES_NOTRDY;
//   if (count != 1) return RES_PARERR; // Only single block supported

//   // if (sd_send_cmd(17, sector, 0x01) != 0x00) {
//   //   sd_cs_high();
//   //   return RES_ERROR;
//   // }
//   volatile uint32_t addr = is_sdhc ? sector : (sector << 9);
//   volatile uint8_t resp = sd_send_cmd(17, addr, 0xFF);

//   // uint8_t resp = sd_send_cmd(17, sector, 0xFF);
//   if (resp != 0x00) {
//     sd_cs_high();
//     printf("sd_read cmd 17 bad response: %x\n\r", resp);
//     // printf("response value: %d, %x\n\r", resp, resp);
//     return RES_ERROR;
//   }
//   // Wait for data token 0xFE
//   for (int i = 0; i < 100000; i++) {
//     uint8_t token = sd_spi_recv();
//     if (token == 0xFE) break;
//     if (i == 99999) {
//       sd_cs_high();
//       return RES_ERROR;
//     }
//   }

//   // Read 512 bytes
//   for (int i = 0; i < 512; i++) {
//     buff[i] = sd_spi_recv();
//   }

//   // Discard CRC
//   sd_spi_recv();
//   sd_spi_recv();

//   sd_cs_high();
//   sd_spi_recv(); // extra clock
//   return RES_OK;
// }

DRESULT sd_read(BYTE *buff, LBA_t sector, UINT count)
{
  if (sd_stat & STA_NOINIT) return RES_NOTRDY;
  if (count != 1) return RES_PARERR;  // Single block only

  uint32_t addr = is_sdhc ? sector : (sector << 9);  // SDHC = block, SDSC = byte addr

  // Send CMD17
  uint8_t resp = sd_send_cmd(17, addr, 0xFF);
  if (resp != 0x00) {
    printf("CMD17 failed: R1 = 0x%02X\n", resp);
    sd_cs_high();
    sd_spi_recv();  // Extra clock
    return RES_ERROR;
  }

  // Wait for data token (0xFE)
  int timeout = 100000;
  uint8_t token;
  do {
    token = sd_spi_recv();
  } while (token == 0xFF && --timeout);

  if (token != 0xFE) {
    printf("Data token error: 0x%02X\n", token);
    sd_cs_high();
    sd_spi_recv();
    return RES_ERROR;
  }

  // Read 512-byte block
  for (int i = 0; i < 512; i++) {
    buff[i] = sd_spi_recv();
  }

  // Read and discard CRC
  sd_spi_recv();
  sd_spi_recv();

  // Wait for card to finish (not busy)
  while (sd_spi_recv() == 0x00);

  // De-select and extra clock
  sd_cs_high();
  sd_spi_recv();

  return RES_OK;
}

DRESULT sd_write(const BYTE *buff, LBA_t sector, UINT count)
{
  if (sd_stat & STA_NOINIT) return RES_NOTRDY;
  if (count != 1) return RES_PARERR; // Only single block supported

  uint8_t resp = sd_send_cmd(24, sector, 0x01);
  if (resp != 0x00) {
    sd_cs_high();
    printf("sd_write error cmd 24: %d\n\r", RES_ERROR);
    printf("response value: %x\n\r", resp);
    return RES_ERROR;
  }

  sd_spi_recv(); // NOP
  sd_spi_send(0xFE); // Start block token

  // Write 512 bytes
  for (int i = 0; i < 512; i++) {
    sd_spi_send(buff[i]);
  }

  // Send dummy CRC
  sd_spi_send(0xFF);
  sd_spi_send(0xFF);

  // Read data response token
  uint8_t response = sd_spi_recv();
  if ((response & 0x1F) != 0x05) {
    sd_cs_high();
    printf("sd_write error response token: %d\n\r", RES_ERROR);
    return RES_ERROR;
  }

  // Wait for write completion
  while (sd_spi_recv() == 0);

  sd_cs_high();
  sd_spi_recv();
  return RES_OK;
}

DSTATUS sd_status(void) {
  return sd_stat;
}
