/**
  ******************************************************************************
  * @file    mx25l12833f.h
  * @brief   MX25L12833F Quad SPI NOR Flash Driver Header
  ******************************************************************************
  */

#ifndef MX25L12833F_H
#define MX25L12833F_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx_hal.h"

/* Flash Parameters */
#define MX25L12833F_FLASH_SIZE      0x1000000   /* 16 MB */
#define MX25L12833F_SECTOR_SIZE     0x1000      /* 4 KB */
#define MX25L12833F_PAGE_SIZE       0x100       /* 256 Bytes */

/* Commands */
#define MX25L12833F_READ_ID_CMD             0x9F
#define MX25L12833F_RESET_ENABLE_CMD        0x66
#define MX25L12833F_RESET_CMD               0x99
#define MX25L12833F_READ_STATUS_REG_CMD     0x05
#define MX25L12833F_WRITE_STATUS_REG_CMD    0x01
#define MX25L12833F_WRITE_ENABLE_CMD        0x06
#define MX25L12833F_READ_CMD                0x03
#define MX25L12833F_PAGE_PROG_CMD           0x02
#define MX25L12833F_SECTOR_ERASE_CMD        0x20
#define MX25L12833F_CHIP_ERASE_CMD          0xC7

/* Status Register Bits */
#define MX25L12833F_SR_WIP                  0x01
#define MX25L12833F_SR_WEL                  0x02
#define MX25L12833F_SR_QE                   0x40

/* IDs */
#define MX25L12833F_MANUFACTURER_ID         0xC2

/* Return Types */
typedef enum {
    MX25L12833F_OK       = 0,
    MX25L12833F_ERROR    = 1,
    MX25L12833F_BUSY     = 2,
    MX25L12833F_TIMEOUT  = 3
} MX25L12833F_StatusTypeDef;

/* Functions */
MX25L12833F_StatusTypeDef MX25L12833F_Init(OSPI_HandleTypeDef *hospi);
MX25L12833F_StatusTypeDef MX25L12833F_ReadID(OSPI_HandleTypeDef *hospi, uint8_t *id);
MX25L12833F_StatusTypeDef MX25L12833F_Read(OSPI_HandleTypeDef *hospi, uint8_t *pData, 
                                            uint32_t ReadAddr, uint32_t Size);
MX25L12833F_StatusTypeDef MX25L12833F_PageProgram(OSPI_HandleTypeDef *hospi, uint8_t *pData, 
                                                   uint32_t WriteAddr, uint32_t Size);
MX25L12833F_StatusTypeDef MX25L12833F_Erase_Sector(OSPI_HandleTypeDef *hospi, uint32_t SectorAddr);
MX25L12833F_StatusTypeDef MX25L12833F_Erase_Chip(OSPI_HandleTypeDef *hospi);
MX25L12833F_StatusTypeDef MX25L12833F_WriteEnable(OSPI_HandleTypeDef *hospi);
MX25L12833F_StatusTypeDef MX25L12833F_AutoPollingMemReady(OSPI_HandleTypeDef *hospi, uint32_t Timeout);

#ifdef __cplusplus
}
#endif

#endif /* MX25L12833F_H */
