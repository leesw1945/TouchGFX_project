/**
  ******************************************************************************
  * @file    Loader_Src.h
  * @author  MCD Application Team
  * @brief   Header file of Loader_Src.c
  ******************************************************************************
  */

#ifndef __LOADER_SRC_H
#define __LOADER_SRC_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"

/* MX25L12833F Commands */
#define RESET_ENABLE_CMD                     0x66
#define RESET_MEMORY_CMD                     0x99

#define READ_CMD                             0x03
#define FAST_READ_CMD                        0x0B
#define DUAL_OUT_READ_CMD                    0x3B
#define DUAL_INOUT_READ_CMD                  0xBB
#define QUAD_OUT_READ_CMD                    0x6B
#define QUAD_INOUT_READ_CMD                  0xEB

#define WRITE_ENABLE_CMD                     0x06
#define WRITE_DISABLE_CMD                    0x04

#define READ_STATUS_REG_CMD                  0x05
#define READ_CFG_REG_CMD                     0x15
#define WRITE_STATUS_CFG_REG_CMD             0x01

#define PAGE_PROG_CMD                        0x02
#define QUAD_PAGE_PROG_CMD                   0x38

#define SECTOR_ERASE_CMD                     0x20
#define BLOCK_ERASE_32K_CMD                  0x52
#define BLOCK_ERASE_64K_CMD                  0xD8
#define CHIP_ERASE_CMD                       0xC7

#define READ_ID_CMD                          0x9F
#define READ_ELECTRONIC_SIGNATURE_CMD        0xAB

#define ENTER_QUAD_MODE_CMD                  0x35
#define EXIT_QUAD_MODE_CMD                   0xF5

/* MX25L12833F Registers */
#define STATUS_REG_WIP_MASK                  0x01  /* Write In Progress */
#define STATUS_REG_WEL_MASK                  0x02  /* Write Enable Latch */

#define CFG_REG_QUAD_MASK                    0x40  /* Quad Enable */

/* Parameters */
#define MEMORY_FLASH_SIZE                    0x01000000  /* 16 MB */
#define MEMORY_BLOCK_SIZE                    0x10000     /* 64KB */
#define MEMORY_SECTOR_SIZE                   0x1000      /* 4KB */
#define MEMORY_PAGE_SIZE                     0x100       /* 256 bytes */

#define DUMMY_CYCLES_READ_QUAD               6
#define DUMMY_CYCLES_READ                    8

/* Memory address base */
#define MEMORY_ADDRESS_BASE                  0x90000000

/* Timeouts */
#define TIMEOUT_VALUE                        1000

/* Compiler specific defines */
#ifdef __ICCARM__
  #define KeepInCompilation __root
#else
  #define KeepInCompilation __attribute__((used))
#endif

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
KeepInCompilation int Init(void);
KeepInCompilation int Write(uint32_t Address, uint32_t Size, uint8_t* buffer);
KeepInCompilation int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress);
KeepInCompilation int MassErase(uint32_t Parallelism);
KeepInCompilation uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr, 
                                   uint32_t Size, uint32_t missalignement);

#ifdef __cplusplus
}
#endif

#endif /* __LOADER_SRC_H */
