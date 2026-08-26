/**
  ******************************************************************************
  * @file    flash_driver.h
  * @brief   MX25L6433F SPI NOR flash read driver + TouchGFX DataReader hooks
  *
  *          User file - not touched by CubeMX / TouchGFX Designer generation.
  ******************************************************************************
  */
#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* JEDEC ID of MX25L6433F: manufacturer 0xC2, type 0x20, density 0x17 (64Mbit) */
#define FLASH_JEDEC_ID_MX25L6433F   0x00C22017UL

/* Probe the flash over SPI2 (JEDEC ID, command 0x9F).
 * Returns 1 when an MX25L6433F answers, 0 otherwise (wiring/power problem). */
int      FLASH_DRIVER_Init(void);
uint32_t FLASH_DRIVER_ReadJEDEC(void);

/* TouchGFX DataReader hooks - called by TouchGFXGeneratedDataReader.cpp.
 * `address24` carries the full virtual address (0x90xxxxxx); the driver
 * masks it down to the 24-bit physical flash address. */
void DataReader_WaitForReceiveDone(void);
void DataReader_ReadData(uint32_t address24, uint8_t *buffer, uint32_t length);
void DataReader_StartDMAReadData(uint32_t address24, uint8_t *buffer, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_DRIVER_H */
