/**
  ******************************************************************************
  * @file    Dev_Inf.h
  * @author  MCD Application Team (Modified for MX25L12833F + STM32U5G9)
  * @brief   Header file of Dev_Inf.c
  *
  * 참고: MX25LM51245G_STM32U575I-EVAL 프로젝트 스타일 적용
  ******************************************************************************
  */

#ifndef __DEV_INF_H
#define __DEV_INF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Device Type Definitions ---------------------------------------------------*/
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

/* Max Number of Sector types */
#define SECTOR_NUM      10

/* ============================================================================
 * StorageInfo 구조체 정의
 *
 * 주의: STM32CubeProgrammer는 이 구조체의 정확한 바이트 레이아웃을 기대합니다.
 * unsigned long/short를 사용하여 호환성 확보 (참고 프로젝트 스타일)
 * ============================================================================ */

/**
 * @brief  Sector information structure
 */
struct DeviceSectors {
    unsigned long SectorNum;     /* Number of Sectors */
    unsigned long SectorSize;    /* Sector Size in Bytes */
};

/**
 * @brief  Storage device information structure
 */
struct StorageInfo {
    char           DeviceName[100];              /* Device Name + Description */
    unsigned short DeviceType;                   /* Device Type: NOR_FLASH, SPI_FLASH, etc. */
    unsigned long  DeviceStartAddress;           /* Device Start Address */
    unsigned long  DeviceSize;                   /* Total Size of Device */
    unsigned long  PageSize;                     /* Programming Page Size */
    unsigned char  EraseValue;                   /* Content of Erased Memory */
    struct DeviceSectors sectors[SECTOR_NUM];    /* Sector Info */
};

/* External declaration for StorageInfo */
extern struct StorageInfo const StorageInfo;

#ifdef __cplusplus
}
#endif

#endif /* __DEV_INF_H */
