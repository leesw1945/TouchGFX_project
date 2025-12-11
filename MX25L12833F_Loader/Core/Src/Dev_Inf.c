/**
  ******************************************************************************
  * @file    Dev_Inf.c
  * @author  MCD Application Team
  * @brief   This file defines the structure containing information about the
  *          external memory MX25L12833F connected to STM32U5G9ZJT6Q.
  ******************************************************************************
  */

#include "Dev_Inf.h"

/* This structure contains information used by ST-LINK Utility to program and erase the device */
#if defined (__ICCARM__)
__root struct StorageInfo const StorageInfo = {
#else
__attribute__((section(".storage_info"))) __attribute__((used))
struct StorageInfo const StorageInfo = {
#endif
  "MX25L12833F_STM32U5G9_CustomBoard",  /* Device Name */
  NOR_FLASH,                             /* Device Type */
  0x90000000,                            /* Device Start Address */
  0x01000000,                            /* Device Size: 16MB (128Mbit) */
  0x00000100,                            /* Programming Page Size: 256 bytes */
  0xFF,                                  /* Initial Content of Erased Memory */
  //{0x00, 0x00, 0x00, 0x00, 0x00},					/* Reserved padding (5 bytes) */

  /* Sector Info: 256 sectors of 64KB each */
  {
    {0x00000100, 0x00010000},            /* 256 Sectors, 64KB each */
	{0x00000000, 0x00000000},
    {0x00000000, 0x00000000},   /* Entry 2: Unused */
    {0x00000000, 0x00000000},   /* Entry 3: Unused */
    {0x00000000, 0x00000000},   /* Entry 4: Unused */
    {0x00000000, 0x00000000},   /* Entry 5: Unused */
    {0x00000000, 0x00000000},   /* Entry 6: Unused */
    {0x00000000, 0x00000000},   /* Entry 7: Unused */
    {0x00000000, 0x00000000},   /* Entry 8: Unused */
    {0x00000000, 0x00000000}    /* Entry 9: Unused (last entry) */
  }
};
