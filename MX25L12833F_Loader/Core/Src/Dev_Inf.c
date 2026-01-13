/**
  ******************************************************************************
  * @file    Dev_Inf.c
  * @author  Fixed Version v3 - 4KB Sector 단위로 수정
  * @brief   MX25L12833F External Loader Device Information
  *
  * ★★★ 수정 사항 (v3) ★★★
  * - Sector 크기를 4KB로 수정 (데이터시트 기준)
  * - MX25L12833F: Sector = 4KB, Block = 64KB
  * - STM32CubeProgrammer의 SectorErase는 Sector 단위 사용
  ******************************************************************************
  */

#include "Dev_Inf.h"

/*
 * MX25L12833F Memory Organization:
 * - Total Size: 16MB (128Mbit)
 * - Sector Size: 4KB (0x1000) - Sector Erase (0x20)
 * - Block Size: 64KB (0x10000) - Block Erase (0xD8)
 * - Page Size: 256 bytes - Page Program (0x02)
 *
 * 16MB / 4KB = 4096 Sectors
 */

#if defined (__ICCARM__)
__root struct StorageInfo const StorageInfo = {
#else
__attribute__((section(".storage_info"))) __attribute__((used)) __attribute__((aligned(4)))
struct StorageInfo const StorageInfo = {
#endif
    "MX25L12833F_STM32U5G9_CustomBoard",  /* Device Name */
    NOR_FLASH,                             /* Device Type: NOR_FLASH = 3 */
    0x90000000,                            /* Device Start Address */
    0x01000000,                            /* Device Size: 16MB (0x1000000) */
    0x00000100,                            /* Page Size: 256 bytes */
    0xFF,                                  /* Erased Memory Value */

    /*
     * ★★★ Sector Info: 4KB Sector 단위 ★★★
     * 4096 Sectors × 4KB = 16MB
     */
    {
        { 0x00001000, 0x00001000 },        /* 4096 (0x1000) Sectors, 4KB (0x1000) each */
        { 0x00000000, 0x00000000 },        /* End marker */
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
