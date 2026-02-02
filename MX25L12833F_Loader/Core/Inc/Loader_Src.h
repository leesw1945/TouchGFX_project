/**
  ******************************************************************************
  * @file    Loader_Src.h
  * @author  MCD Application Team (Modified for MX25L12833F + STM32U5G9)
  * @brief   Header file of Loader_Src.c for MX25L12833F
  *
  * 참고: MX25LM51245G_STM32U575I-EVAL 프로젝트 스타일 적용
  ******************************************************************************
  */

#ifndef __LOADER_SRC_H
#define __LOADER_SRC_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_ospi.h"

/* ============================================================================
 * MX25L12833F Commands
 * ============================================================================ */
/* Reset Commands */
#define RESET_ENABLE_CMD                     0x66
#define RESET_MEMORY_CMD                     0x99

/* Read Commands */
#define READ_CMD                             0x03    /* Normal Read (1-1-1) */
#define FAST_READ_CMD                        0x0B    /* Fast Read */
#define DUAL_OUT_READ_CMD                    0x3B    /* Dual Output Read */
#define DUAL_INOUT_READ_CMD                  0xBB    /* Dual I/O Read */
#define QUAD_OUT_READ_CMD                    0x6B    /* Quad Output Read */
#define QUAD_INOUT_READ_CMD                  0xEB    /* Quad I/O Read */

/* Write Commands */
#define WRITE_ENABLE_CMD                     0x06    /* Write Enable */
#define WRITE_DISABLE_CMD                    0x04    /* Write Disable */

/* Register Commands */
#define READ_STATUS_REG_CMD                  0x05    /* Read Status Register */
#define WRITE_STATUS_REG_CMD                 0x01    /* Write Status Register */
#define READ_CONFIG_REG_CMD                  0x15    /* Read Configuration Register */
#define READ_SECURITY_REG_CMD                0x2B    /* Read Security Register (RDSCUR) */

/* Program Commands */
#define PAGE_PROG_CMD                        0x02    /* Page Program (1-1-1) */
#define QUAD_PAGE_PROG_CMD                   0x38    /* Quad Page Program */

/* Erase Commands */
#define SECTOR_ERASE_CMD                     0x20    /* Sector Erase 4KB */
#define BLOCK_ERASE_32K_CMD                  0x52    /* Block Erase 32KB */
#define BLOCK_ERASE_64K_CMD                  0xD8    /* Block Erase 64KB */
#define CHIP_ERASE_CMD                       0xC7    /* Chip Erase */

/* ID Commands */
#define READ_ID_CMD                          0x9F    /* Read ID (RDID) */
#define READ_ELECTRONIC_SIGNATURE_CMD        0xAB    /* Read Electronic Signature */

/* Quad Mode Commands */
#define ENTER_QUAD_MODE_CMD                  0x35    /* Enter QPI Mode */
#define EXIT_QUAD_MODE_CMD                   0xF5    /* Exit QPI Mode */

/* Protection Commands */
#define GANG_BLOCK_UNLOCK_CMD                0x98    /* Gang Block Unlock (GBULK) */

/* ============================================================================
 * Status Register Bit Masks
 * ============================================================================ */
#define STATUS_REG_WIP_MASK                  0x01    /* bit0: Write In Progress */
#define STATUS_REG_WEL_MASK                  0x02    /* bit1: Write Enable Latch */
#define STATUS_REG_BP0_MASK                  0x04    /* bit2: Block Protect 0 */
#define STATUS_REG_BP1_MASK                  0x08    /* bit3: Block Protect 1 */
#define STATUS_REG_BP2_MASK                  0x10    /* bit4: Block Protect 2 */
#define STATUS_REG_BP3_MASK                  0x20    /* bit5: Block Protect 3 */
#define STATUS_REG_QE_MASK                   0x40    /* bit6: Quad Enable */
#define STATUS_REG_SRWD_MASK                 0x80    /* bit7: Status Register Write Disable */
#define STATUS_REG_BP_MASK                   0x3C    /* bit5-2: All BP bits */

/* ============================================================================
 * Security Register Bit Masks
 * ============================================================================ */
#define SECURITY_REG_OTP_LOCK_MASK           0x03    /* bit1-0: OTP Lock */
#define SECURITY_REG_LDSO_MASK               0x02    /* bit1: Lock-down Secured OTP */
#define SECURITY_REG_PSB_MASK                0x04    /* bit2: Program Suspend */
#define SECURITY_REG_ESB_MASK                0x08    /* bit3: Erase Suspend */
#define SECURITY_REG_P_FAIL_MASK             0x20    /* bit5: Program Fail */
#define SECURITY_REG_E_FAIL_MASK             0x40    /* bit6: Erase Fail */
#define SECURITY_REG_WPSEL_MASK              0x80    /* bit7: Write Protect Selection */

/* ============================================================================
 * Memory Parameters
 * ============================================================================ */
#define MEMORY_FLASH_SIZE                    0x01000000  /* 16 MB (128 Mbit) */
#define MEMORY_BLOCK_SIZE                    0x10000     /* 64KB Block */
#define MEMORY_SECTOR_SIZE                   0x1000      /* 4KB Sector */
#define MEMORY_PAGE_SIZE                     0x100       /* 256 bytes Page */

/* ============================================================================
 * Timing Parameters (ms) - from datasheet
 * ============================================================================ */
#define TIMEOUT_WRITE_STATUS_REG             100     /* tW: max 40ms */
#define TIMEOUT_PAGE_PROGRAM                 10      /* tPP: max 1.2ms */
#define TIMEOUT_SECTOR_ERASE_4K              200     /* tSE: max 120ms */
#define TIMEOUT_BLOCK_ERASE_32K              1000    /* tBE32: max 650ms */
#define TIMEOUT_BLOCK_ERASE_64K              1000    /* tBE: max 650ms */
#define TIMEOUT_CHIP_ERASE                   120000  /* tCE: max 60s */

/* Dummy Cycles */
#define DUMMY_CYCLES_READ_QUAD               6
#define DUMMY_CYCLES_READ                    8

/* Memory address base */
#define MEMORY_ADDRESS_BASE                  0x90000000

/* General timeout */
#define TIMEOUT_VALUE                        1000

/* ============================================================================
 * Device ID
 * ============================================================================ */
#define MX25L12833F_MANUFACTURER_ID          0xC2
#define MX25L12833F_MEMORY_TYPE              0x20
#define MX25L12833F_MEMORY_DENSITY           0x18

/* ============================================================================
 * Compiler Specific Defines
 * ============================================================================ */
#ifdef __ICCARM__
  #define KeepInCompilation __root
#else
  #define KeepInCompilation __attribute__((used))
#endif

/* ============================================================================
 * Exported Functions (External Loader API)
 * ============================================================================ */
/* 필수 함수들 - STM32CubeProgrammer가 호출 */
KeepInCompilation int Init(void);
KeepInCompilation int Write(uint32_t Address, uint32_t Size, uint8_t* buffer);
KeepInCompilation int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress);
KeepInCompilation int MassErase(uint32_t Parallelism);
KeepInCompilation uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr,
                                   uint32_t Size, uint32_t missalignement);

/* 선택적 함수 */
KeepInCompilation int Read(uint32_t Address, uint32_t Size, uint8_t* buffer);

/* HAL Tick Override - External Loader에서 SysTick 사용 안함 */
KeepInCompilation HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority);

#ifdef __cplusplus
}
#endif

#endif /* __LOADER_SRC_H */
