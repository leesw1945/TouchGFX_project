/**
  ******************************************************************************
  * @file    Dev_Inf.c
  * @author  MCD Application Team (Modified for MX25L12833F + STM32U5G9)
  * @brief   This file defines the structure containing information about the
  *          external flash memory MX25L12833F used by STM32CubeProgrammer in
  *          programming/erasing the device.
  *
  * 참고: MX25LM51245G_STM32U575I-EVAL 프로젝트 스타일 적용
  ******************************************************************************
  */

#include "Dev_Inf.h"

/*
 * MX25L12833F Memory Organization:
 * ================================
 * - Total Size: 16MB (128Mbit)
 * - Sector Size: 4KB (0x1000) - Sector Erase (0x20)
 * - Block Size: 64KB (0x10000) - Block Erase (0xD8)
 * - Page Size: 256 bytes - Page Program (0x02)
 *
 * 16MB / 4KB = 4096 Sectors
 *
 * 비교 (참고 프로젝트 MX25LM51245G):
 * - MX25LM51245G: 64MB, 64KB sector, OPI DTR
 * - MX25L12833F: 16MB, 4KB sector, SPI 1-line
 */

/* This structure contains information used by STM32CubeProgrammer to program and erase the device */
#if defined (__ICCARM__)
__root struct StorageInfo const StorageInfo = {
#else
struct StorageInfo const StorageInfo = {
#endif
   "MX25L12833F_STM32U5G9ZJT6Q",          /* Device Name + Board name */
   NOR_FLASH,                              /* Device Type */
   0x90000000,                             /* Device Start Address (OCTOSPI1 memory-mapped) */
   0x01000000,                             /* Device Size: 16MB (0x1000000 = 16,777,216) */
   0x100,                                  /* Programming Page Size: 256 Bytes */
   0xFF,                                   /* Initial Content of Erased Memory */
   {
/* Specify Size and Address of Sectors (view example below) */
		   {0x00001000, 0x00001000},                 /* Sector Num: 4096 (0x1000), Sector Size: 4KB (0x1000) */
		   {0x00000000, 0x00000000},                 /* End marker */
   }
};
