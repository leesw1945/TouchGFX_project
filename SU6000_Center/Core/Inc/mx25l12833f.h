/**
  ******************************************************************************
  * @file    mx25l12833f.h
  * @brief   MX25L12833F QSPI Flash Driver Header
  ******************************************************************************
  */

#ifndef __MX25L12833F_H
#define __MX25L12833F_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"

/* MX25L12833F Configuration -------------------------------------------------*/
#define MX25L12833F_FLASH_SIZE          0x1000000  /* 128 Mbit => 16 MBytes */
#define MX25L12833F_SECTOR_SIZE         0x1000     /* 4096 sectors of 4 KBytes */
#define MX25L12833F_SUBSECTOR_SIZE      0x1000     /* 4096 subsectors of 4 KBytes */
#define MX25L12833F_PAGE_SIZE           0x100      /* 65536 pages of 256 bytes */
#define MX25L12833F_BLOCK_SIZE          0x10000    /* 256 blocks of 64 KBytes */

#define MX25L12833F_DUMMY_CYCLES_READ_QUAD    4    /* Dummy cycles for Quad I/O Read (0xEB) */

/* MX25L12833F Commands ------------------------------------------------------*/
/* Reset Operations */
#define RESET_ENABLE_CMD                        0x66
#define RESET_MEMORY_CMD                        0x99

/* Identification Operations */
#define READ_ID_CMD                             0x9F
#define READ_ELECTRONIC_ID_CMD                  0xAB
#define READ_ELEC_MANUFACTURER_DEVICE_ID_CMD    0x90

/* Read Operations */
#define READ_CMD                                0x03
#define FAST_READ_CMD                           0x0B
#define DUAL_OUT_FAST_READ_CMD                  0x3B
#define DUAL_INOUT_FAST_READ_CMD                0xBB
#define QUAD_OUT_FAST_READ_CMD                  0x6B
#define QUAD_INOUT_FAST_READ_CMD                0xEB

/* Write Operations */
#define WRITE_ENABLE_CMD                        0x06
#define WRITE_DISABLE_CMD                       0x04

/* Register Operations */
#define READ_STATUS_REG_CMD                     0x05
#define READ_CONFIG_REG_CMD                     0x15
#define WRITE_STATUS_REG_CMD                    0x01
#define WRITE_CONFIG_REG_CMD                    0x01

/* Program Operations */
#define PAGE_PROG_CMD                           0x02
#define QUAD_PAGE_PROG_CMD                      0x38

/* Erase Operations */
#define SECTOR_ERASE_CMD                        0x20
#define BLOCK_ERASE_32K_CMD                     0x52
#define BLOCK_ERASE_64K_CMD                     0xD8
#define CHIP_ERASE_CMD                          0xC7
#define CHIP_ERASE_CMD_2                        0x60

/* Status Register Bits */
#define MX25L12833F_SR_WIP                      0x01  /* Write in Progress */
#define MX25L12833F_SR_WEL                      0x02  /* Write Enable Latch */
#define MX25L12833F_SR_BP0                      0x04  /* Block Protect 0 */
#define MX25L12833F_SR_BP1                      0x08  /* Block Protect 1 */
#define MX25L12833F_SR_BP2                      0x10  /* Block Protect 2 */
#define MX25L12833F_SR_BP3                      0x20  /* Block Protect 3 */
#define MX25L12833F_SR_QE                       0x40  /* Quad Enable */
#define MX25L12833F_SR_SRWD                     0x80  /* Status Register Write Disable */

/* Timing Specifications (worst case, milliseconds) */
#define MX25L12833F_WRITE_REG_MAX_TIME          40
#define MX25L12833F_SECTOR_ERASE_MAX_TIME       400
#define MX25L12833F_BLOCK_ERASE_32K_MAX_TIME    1600
#define MX25L12833F_BLOCK_ERASE_64K_MAX_TIME    2000
#define MX25L12833F_CHIP_ERASE_MAX_TIME         40000
#define MX25L12833F_PAGE_PROG_MAX_TIME          5

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  MX25L12833F_OK            = 0,
  MX25L12833F_ERROR         = 1,
  MX25L12833F_BUSY          = 2,
  MX25L12833F_TIMEOUT       = 3
} MX25L12833F_Status_t;

typedef struct
{
  uint32_t FlashSize;
  uint32_t SectorSize;
  uint32_t SectorsNumber;
  uint32_t PageSize;
  uint32_t PagesNumber;
} MX25L12833F_Info_t;

/* Exported functions --------------------------------------------------------*/
/* Initialization and Reset */
MX25L12833F_Status_t MX25L12833F_Init(OSPI_HandleTypeDef *hospi);
MX25L12833F_Status_t MX25L12833F_Reset(OSPI_HandleTypeDef *hospi);
MX25L12833F_Status_t MX25L12833F_GetInfo(MX25L12833F_Info_t *pInfo);

/* Configuration */
MX25L12833F_Status_t MX25L12833F_EnableQuadMode(OSPI_HandleTypeDef *hospi);
MX25L12833F_Status_t MX25L12833F_DisableQuadMode(OSPI_HandleTypeDef *hospi);
MX25L12833F_Status_t MX25L12833F_EnableMemoryMappedMode(OSPI_HandleTypeDef *hospi);

/* Read/Write Operations */
MX25L12833F_Status_t MX25L12833F_Read(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
MX25L12833F_Status_t MX25L12833F_Write(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
MX25L12833F_Status_t MX25L12833F_Erase_Sector(OSPI_HandleTypeDef *hospi, uint32_t SectorAddress);
MX25L12833F_Status_t MX25L12833F_Erase_Block(OSPI_HandleTypeDef *hospi, uint32_t BlockAddress);
MX25L12833F_Status_t MX25L12833F_Erase_Chip(OSPI_HandleTypeDef *hospi);

/* Low Level Functions */
MX25L12833F_Status_t MX25L12833F_WriteEnable(OSPI_HandleTypeDef *hospi);
MX25L12833F_Status_t MX25L12833F_WriteDisable(OSPI_HandleTypeDef *hospi);
MX25L12833F_Status_t MX25L12833F_ReadStatusRegister(OSPI_HandleTypeDef *hospi, uint8_t *pData);
MX25L12833F_Status_t MX25L12833F_WaitForWriteEnd(OSPI_HandleTypeDef *hospi, uint32_t Timeout);
MX25L12833F_Status_t MX25L12833F_ReadID(OSPI_HandleTypeDef *hospi, uint8_t *pData);

#ifdef __cplusplus
}
#endif

#endif /* __MX25L12833F_H */
