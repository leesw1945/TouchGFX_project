/**
  ******************************************************************************
  * @file    flash_driver.c
  * @brief   MX25L6433F SPI NOR flash read driver + TouchGFX DataReader hooks
  *
  *          User file - not touched by CubeMX / TouchGFX Designer generation.
  *
  *          Wiring (NUCLEO-G0B1RE + X-NUCLEO-GFX01M2):
  *            SPI2 (PB13 SCK / PC2 MISO / PC3 MOSI), 32 Mbit/s
  *            SPI2_NCS (PA8) : flash chip select, active low
  *            TX DMA = DMA1_Channel3, RX DMA = DMA1_Channel2
  *
  *          TouchGFX renders UI while streaming image/font data from this
  *          flash: TouchGFXGeneratedDataReader issues FAST READ (0x0B)
  *          requests through the three DataReader_* hooks below. Two line
  *          buffers are used so the DMA read of line N+1 overlaps the
  *          rendering of line N.
  ******************************************************************************
  */
#include "flash_driver.h"
#include "main.h"
#include "spi.h"

/* MX25L6433F commands used here (read path only - programming is done by the
 * STM32CubeProgrammer external loader, see FlashLoader/) */
#define FLASH_CMD_RDID       0x9FU   /* Read JEDEC ID                       */
#define FLASH_CMD_FAST_READ  0x0BU   /* Fast Read: cmd + addr24 + 1 dummy   */

#define FLASH_ADDR_MASK      0x00FFFFFFUL

static volatile int rx_busy = 0;     /* a DMA read is in flight             */

static inline void FLASH_CS_Low(void)
{
    HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_RESET);
}

static inline void FLASH_CS_High(void)
{
    HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_SET);
}

/* Send FAST READ header: command, 24-bit address (MSB first), 1 dummy byte.
 * CS stays low afterwards - data phase follows. */
static void FLASH_SendReadHeader(uint32_t addr)
{
    uint8_t hdr[5] = {
        FLASH_CMD_FAST_READ,
        (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr,
        0x00
    };
    FLASH_CS_Low();
    HAL_SPI_Transmit(&hspi2, hdr, sizeof(hdr), HAL_MAX_DELAY);
}

uint32_t FLASH_DRIVER_ReadJEDEC(void)
{
    uint8_t cmd = FLASH_CMD_RDID;
    uint8_t id[3] = { 0 };

    FLASH_CS_Low();
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi2, id, sizeof(id), HAL_MAX_DELAY);
    FLASH_CS_High();

    return ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
}

int FLASH_DRIVER_Init(void)
{
    FLASH_CS_High();
    return (FLASH_DRIVER_ReadJEDEC() == FLASH_JEDEC_ID_MX25L6433F) ? 1 : 0;
}

/* TouchGFX DataReader hooks ---------------------------------------------------*/

/* Small transfers: blocking read on the CPU (faster than DMA setup overhead) */
void DataReader_ReadData(uint32_t address24, uint8_t *buffer, uint32_t length)
{
    FLASH_SendReadHeader(address24 & FLASH_ADDR_MASK);
    HAL_SPI_Receive(&hspi2, buffer, (uint16_t)length, HAL_MAX_DELAY);
    FLASH_CS_High();
}

/* Large transfers: start an RX DMA read and return immediately - TouchGFX
 * keeps rendering the previous line while this one arrives. */
void DataReader_StartDMAReadData(uint32_t address24, uint8_t *buffer, uint32_t length)
{
    rx_busy = 1;
    FLASH_SendReadHeader(address24 & FLASH_ADDR_MASK);
    HAL_SPI_Receive_DMA(&hspi2, buffer, (uint16_t)length);
}

void DataReader_WaitForReceiveDone(void)
{
    while (rx_busy)
    {
    }
}

/* SPI RX DMA complete. SPI2 = flash read finished.
 * (HAL routes full-duplex master receive-DMA completion here.) */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2)
    {
        FLASH_CS_High();
        rx_busy = 0;
    }
}
