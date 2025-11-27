/**
  ******************************************************************************
  * @file    Dev_Inf.c
  * @brief   Device Information for External Loader
  *          MX25L12833F 16MB Quad SPI Flash on STM32U5G9ZJT6Q
  ******************************************************************************
  */

#include "Dev_Inf.h"

/**
  * @brief  Storage Information Structure
  * @note   MUST be placed at address 0x20000004
  *         The attribute places it in .rodata.StorageInfo section
  */
#if defined(__ICCARM__)
#pragma location = ".rodata.StorageInfo"
__root
#elif defined(__CC_ARM)
__attribute__((section(".rodata.StorageInfo"), used))
#elif defined(__GNUC__)
__attribute__((section(".rodata.StorageInfo"), used))
#endif
struct StorageInfo const StorageInfo = {
    "MX25L12833F_STM32U5G9-Center", /* Device Name (max 100 chars) */
    NOR_FLASH,                       /* Device Type */
    0x90000000,                      /* Device Start Address */
    0x01000000,                      /* Device Size: 16MB */
    0x100,                           /* Page Size: 256 bytes */
    0xFF,                            /* Erased byte value */
    {
        /* Sector Info: {Count, Size} pairs, terminated by {0,0} */
        { 0x1000, 0x00001000 },      /* 4096 sectors x 4KB = 16MB */
        { 0x0000, 0x00000000 }       /* End marker */
    }
};
