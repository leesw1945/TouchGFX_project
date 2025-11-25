/**
  ******************************************************************************
  * @file    Dev_Inf.c
  * @author  MCD Application Team (modified for MX25L12833F)
  * @brief   This file defines the structure containing informations about the 
  *          external flash memory MX25L12833F used by STM32CubeProgramer in 
  *          programming/erasing the device.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Force C linkage for StorageInfo */
#ifdef __cplusplus
extern "C" {
#endif

__attribute__((section(".rodata"))) __attribute__((used))
const struct StorageInfo StorageInfo = {
    /* ... */
};

#ifdef __cplusplus
}
#endif

#include "Dev_Inf.h"

/* This structure contains information used by ST-LINK Utility to program and erase the device */
#if defined (__ICCARM__)
__root struct StorageInfo const StorageInfo  =  {
#else
struct StorageInfo const StorageInfo  =  {
#endif
   "MX25L12833F_STM32U5G9ZJT6Q",                        // Device Name + MCU name
   NOR_FLASH,                                           // Device Type
   0x90000000,                                          // Device Start Address (OCTOSPI memory-mapped region)
   0x1000000,                                           // Device Size: 16 MBytes (128 Mbits)
   0x100,                                               // Programming Page Size: 256 Bytes
   0xFF,                                                // Initial Content of Erased Memory
   
   // Specify Size and Address of Sectors
   // MX25L12833F has:
   // - 4KB sectors (0x1000 bytes)
   // - 64KB blocks (0x10000 bytes)
   // Using 64KB block erase for efficiency
   0x00000100, 0x00010000,                              // Sector Num: 256, Sector Size: 64 KBytes
   0x00000000, 0x00000000,      
}; 
