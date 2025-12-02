/**
  ******************************************************************************
  * @file    Dev_Inf.h
  * @author  MCD Application Team
  * @brief   This file contains the external memory device information.
  ******************************************************************************
  */

#ifndef __DEV_INF_H
#define __DEV_INF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#define SECTOR_NUM           1                  /* Number of sector types */

#define PAGE_SIZE            (uint32_t)(256)    /* 256 bytes */
#define SECTOR_SIZE          (uint32_t)(4096)   /* 4KB sectors */
#define BLOCK_SIZE           (uint32_t)(65536)  /* 64KB blocks */

/* Exported types ------------------------------------------------------------*/
typedef enum {
  SECTOR_PROTECTED   = 0,
  SECTOR_UNPROTECTED = 1,
  SECTOR_DONT_CARE   = 2
} SectorProtectionTypeDef;

typedef enum {
  SPI_FLASH    = 1,
  SPI_RAM      = 2,
  NOR_FLASH    = 3,
  NAND_FLASH   = 4,
  SDRAM        = 5,
  SRAM         = 6,
} DeviceTypeDef;

struct StorageInfo  {
  char            DeviceName[100];         /* Device Name and Description */
  DeviceTypeDef   DeviceType;              /* Device Type: ONCHIP, NOR, NAND, RAM, FLASH, PSRAM ... */
  uint32_t        DeviceStartAddress;      /* Default Device Start Address */
  uint32_t        DeviceSize;              /* Total Size of Device */
  uint32_t        PageSize;                /* Programming Page Size */
  uint8_t         EraseValue;              /* Content of Erased Memory */

  struct  {
    uint32_t SectorNum;                    /* Number of Sectors */
    uint32_t SectorSize;                   /* Sector Size in Bytes */
    SectorProtectionTypeDef SectorProtection;
  } SectorInfo[SECTOR_NUM];
};

#ifdef __cplusplus
}
#endif

#endif /* __DEV_INF_H */
