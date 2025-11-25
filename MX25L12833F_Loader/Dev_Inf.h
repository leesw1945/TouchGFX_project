/**
  ******************************************************************************
  * @file    Dev_Inf.h
  * @author  MCD Application Team
  * @brief   Header file for Dev_Inf.c
  ******************************************************************************
  */

#ifndef __DEV_INF_H
#define __DEV_INF_H

/* Device Type */
#define NOR_FLASH      1
#define NAND_FLASH     2
#define SERIAL_FLASH   3
#define SRAM           4
#define PSRAM          5
#define PC_CARD        6
#define SPI_FLASH      7
#define I2C_FLASH      8
#define SDRAM          9
#define I2C_EEPROM     10

/* StorageInfo structure */
struct StorageInfo
{
  char DeviceName[100];    /* Device Name and Description */
  unsigned short DeviceType;    /* Device Type: ONCHIP, EXT8BIT, EXT16BIT, ... */
  unsigned int DeviceStartAddress; /* Default Device Start Address */
  unsigned int DeviceSize;    /* Total Size of Device */
  unsigned int PageSize;    /* Programming Page Size */
  unsigned char EraseValue;    /* Content of Erased Memory */
  unsigned int sectors_Nb;    /* Number of sectors */
  unsigned int SectorSize;    /* Size of Sector */
  unsigned int Bank1_End;     /* End of Bank1 */
  unsigned int Bank2_End;     /* End of Bank2 */
};

#endif /* __DEV_INF_H */
