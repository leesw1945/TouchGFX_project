/**
  ******************************************************************************
  * @file    Dev_Inf.h
  * @brief   Device Information Header for External Loader
  ******************************************************************************
  */

#ifndef DEV_INF_H
#define DEV_INF_H

#include <stdint.h>

/* Device Type Definitions */
#define MCU_FLASH           0x0001
#define NAND_FLASH          0x0002
#define NOR_FLASH           0x0004
#define SRAM                0x0008
#define PSRAM               0x0010
#define PC_CARD             0x0020
#define SPI_FLASH           0x0040
#define I2C_FLASH           0x0080
#define SDRAM               0x0100
#define I2C_EEPROM          0x0200

/* Sector Info Structure */
struct DeviceSectors
{
    uint32_t SectorNum;     /* Number of Sectors */
    uint32_t SectorSize;    /* Sector Size in Bytes */
};

/* Storage Info Structure - Must match STM32CubeProgrammer expectations */
struct StorageInfo
{
    char           DeviceName[100];     /* Device Name */
    uint16_t       DeviceType;          /* Device Type */
    uint32_t       DeviceStartAddress;  /* Start Address */
    uint32_t       DeviceSize;          /* Device Size */
    uint32_t       PageSize;            /* Page Size */
    uint8_t        EraseVal;            /* Erased Memory Value */
    struct DeviceSectors sectors[10];   /* Sector Info */
};

extern struct StorageInfo const StorageInfo;

#endif /* DEV_INF_H */
