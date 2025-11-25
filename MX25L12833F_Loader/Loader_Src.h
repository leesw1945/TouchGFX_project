/**
  ******************************************************************************
  * @file    Loader_Src.h
  * @author  MCD Application Team
  * @brief   Header file for Loader_Src.c
  ******************************************************************************
  */

#ifndef __LOADER_SRC_H
#define __LOADER_SRC_H

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx.h"
#include "stm32u5xx_hal.h"
#include "Dev_Inf.h"

/* Exported defines ----------------------------------------------------------*/
/* This value can be equal to (512 * 1024) or (1024 * 1024) bytes */
#define PROG_BUFFER_SIZE (1024 * 1024)

/* Keep functions in compilation */
#define KeepInCompilation __attribute__((used))

/* Exported functions prototypes ---------------------------------------------*/
int Init(void);
KeepInCompilation int Write(uint32_t Address, uint32_t Size, uint8_t* buffer);
KeepInCompilation int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress);
KeepInCompilation int MassErase(uint32_t Parallelism);
uint32_t CheckSum(uint32_t StartAddress, uint32_t Size, uint32_t InitVal);
KeepInCompilation uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr, 
                                  uint32_t Size, uint32_t missalignement);

/* HAL System Configuration --------------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);

#endif /* __LOADER_SRC_H */
