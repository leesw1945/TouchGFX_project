/**
  ******************************************************************************
  * @file    Dev_Inf.c
  * @author  Fixed Version
  * @brief   This file defines the structure containing information about the
  *          external memory MX25L12833F connected to STM32U5G9ZJT6Q.
  *
  * CRITICAL: StorageInfo 구조체는 STM32CubeProgrammer와 정확히 일치해야 함
  ******************************************************************************
  */

#include "Dev_Inf.h"

/*
 * This structure contains information used by ST-LINK Utility to program and erase the device
 *
 * STM32CubeProgrammer가 기대하는 StorageInfo 구조체 레이아웃:
 * - DeviceName[100]      : 100 bytes (offset 0)
 * - DeviceType           : 2 bytes   (offset 100) - MUST be uint16_t
 * - DeviceStartAddress   : 4 bytes   (offset 102)
 * - DeviceSize           : 4 bytes   (offset 106)
 * - PageSize             : 4 bytes   (offset 110)
 * - EraseValue           : 1 byte    (offset 114)
 * - Reserved/Padding     : 5 bytes   (offset 115) - alignment padding
 * - SectorInfo[10]       : 80 bytes  (offset 120) - 10 * (4+4)
 * Total: 200 bytes
 */

#if defined (__ICCARM__)
__root struct StorageInfo const StorageInfo = {
#else
__attribute__((section(".storage_info"))) __attribute__((used)) __attribute__((aligned(4)))
struct StorageInfo const StorageInfo = {
#endif
    "MX25L12833F_STM32U5G9_CustomBoard",  /* Device Name - max 100 chars */
    NOR_FLASH,                             /* Device Type: NOR_FLASH = 3 */
    0x90000000,                            /* Device Start Address */
    0x01000000,                            /* Device Size: 16MB (128Mbit) */
    0x00000100,                            /* Programming Page Size: 256 bytes */
    0xFF,                                  /* Initial Content of Erased Memory */

    /* Sector Info: 256 sectors of 64KB each = 16MB total */
    {
        { 0x00001000, 0x00001000 },        /* 256 Sectors, 64KB (0x10000) each */
        { 0x00000000, 0x00000000 },        /* End marker - required! */
        { 0x00000000, 0x00000000 },
        { 0x00000000, 0x00000000 },
        { 0x00000000, 0x00000000 },
        { 0x00000000, 0x00000000 },
        { 0x00000000, 0x00000000 },
        { 0x00000000, 0x00000000 },
        { 0x00000000, 0x00000000 },
        { 0x00000000, 0x00000000 }
    }
};
