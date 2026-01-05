/**
  ******************************************************************************
  * @file    Dev_Inf.h
  * @author  Fixed Version
  * @brief   This file contains the external memory device information.
  *
  * CRITICAL: 구조체가 packed 되어야 STM32CubeProgrammer와 호환됨
  ******************************************************************************
  */

#ifndef __DEV_INF_H
#define __DEV_INF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Device Type Definitions */
#define MCU_FLASH       1
#define NAND_FLASH      2
#define NOR_FLASH       3
#define SRAM            4
#define PSRAM           5
#define PC_CARD         6
#define SPI_FLASH       7
#define I2C_FLASH       8
#define SDRAM           9
#define I2C_EEPROM      10

#define SECTOR_NUM      10                  /* Number of sector types */

/* Memory Size Definitions */
#define PAGE_SIZE       (uint32_t)(256)     /* 256 bytes */
#define SECTOR_SIZE     (uint32_t)(4096)    /* 4KB sectors */
#define BLOCK_SIZE      (uint32_t)(65536)   /* 64KB blocks */

/* ============================================================================
 * StorageInfo 구조체 정의
 *
 * 주의: STM32CubeProgrammer는 이 구조체의 정확한 바이트 레이아웃을 기대합니다.
 * packed 속성을 사용하여 컴파일러가 패딩을 추가하지 않도록 합니다.
 * ============================================================================ */

/**
 * @brief  Sector information structure
 */
//#pragma pack(push, 1)  /* 패킹 시작 - 패딩 없음 */

struct DeviceSectors {
    uint32_t SectorNum;      /* Number of Sectors */
    uint32_t SectorSize;     /* Sector Size in Bytes */
};

struct StorageInfo {
    char          DeviceName[100];              /* Device Name + Description (100 bytes) */
    uint16_t      DeviceType;                   /* Device Type (2 bytes) */
    uint32_t      DeviceStartAddress;           /* Device Start Address (4 bytes) */
    uint32_t      DeviceSize;                   /* Total Size of Device (4 bytes) */
    uint32_t      PageSize;                     /* Programming Page Size (4 bytes) */
    uint8_t       EraseValue;                   /* Content of Erased Memory (1 byte) */
    //uint8_t       Reserved[3];                  /* Reserved/Padding (3 bytes) - alignment */
    struct DeviceSectors SectorInfo[SECTOR_NUM]; /* Sector Info (80 bytes) */
};

//#pragma pack(pop)  /* 패킹 종료 */

/* External declaration for StorageInfo */
extern struct StorageInfo const StorageInfo;

#ifdef __cplusplus
}
#endif

#endif /* __DEV_INF_H */
