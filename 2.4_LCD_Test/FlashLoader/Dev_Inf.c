/**
  ******************************************************************************
  * @file    Dev_Inf.c
  * @brief   Device information for STM32CubeProgrammer:
  *          MX25L6433F (64Mbit SPI NOR) on STM32G0B1 via SPI2.
  *
  *          MX25L6433F organization:
  *            - total 8MB (0x800000), mapped virtually at 0x90000000
  *            - 2048 sectors x 4KB (sector erase 0x20)
  *            - page program 256 bytes (0x02)
  ******************************************************************************
  */
#include "Dev_Inf.h"

#if defined (__ICCARM__)
__root struct StorageInfo const StorageInfo = {
#else
struct StorageInfo const StorageInfo = {
#endif
    "MX25L6433F_2.4LCD_G0B1",               /* Device name + board          */
    NOR_FLASH,                              /* Device type                  */
    0x90000000,                             /* Device start (virtual) addr  */
    0x00800000,                             /* Device size: 8MB             */
    0x100,                                  /* Programming page size: 256B  */
    0xFF,                                   /* Content of erased memory     */
    {
        { 0x00000800, 0x00001000 },         /* 2048 sectors x 4KB           */
        { 0x00000000, 0x00000000 },         /* end marker                   */
    }
};
