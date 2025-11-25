/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @author  MCD Application Team (modified for MX25L12833F)
  * @brief   This file defines the operations of the external loader for
  *          MX25L12833F QSPI memory on STM32U5G9ZJT6Q.
  *           
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
/* Force C linkage for all functions */
#ifdef __cplusplus
extern "C" {
#endif

/* 모든 함수 정의 */

#ifdef __cplusplus
}
#endif

#include "Loader_Src.h"
#include "stm32u5xx_hal.h"
#include <string.h>

#pragma section=".bss"

/* Private defines -----------------------------------------------------------*/
#define TIMEOUT_VALUE              5000

/* MX25L12833F Commands */
#define RESET_ENABLE_CMD           0x66
#define RESET_MEMORY_CMD           0x99
#define READ_ID_CMD                0x9F
#define READ_STATUS_REG_CMD        0x05
#define WRITE_STATUS_REG_CMD       0x01
#define WRITE_ENABLE_CMD           0x06
#define WRITE_DISABLE_CMD          0x04
#define SECTOR_ERASE_4K_CMD        0x20
#define BLOCK_ERASE_64K_CMD        0xD8
#define CHIP_ERASE_CMD             0xC7
#define QUAD_PAGE_PROG_CMD         0x32
#define QUAD_INOUT_FAST_READ_CMD   0xEB

/* MX25L12833F Configuration */
#define MX25L12833F_PAGE_SIZE      0x100      /* 256 bytes */
#define MX25L12833F_BLOCK_SIZE     0x10000    /* 64KB */
#define MX25L12833F_SECTOR_SIZE    0x1000     /* 4KB */
#define MX25L12833F_FLASH_SIZE     0x1000000  /* 16MB */
#define MX25L12833F_DUMMY_CYCLES_READ_QUAD  4
#define MEMORY_MAPPED_ADDRESS     0x90000000

/* Private variables ---------------------------------------------------------*/
OSPI_HandleTypeDef hospi1;

/* Private function prototypes -----------------------------------------------*/
static int SystemClock_Config(void);
static int MX_OCTOSPI1_Init(void);
static int MX25L12833F_Reset(void);
static int MX25L12833F_EnableQuadMode(void);
static int MX25L12833F_WriteEnable(void);
static int MX25L12833F_WaitForWriteEnd(uint32_t Timeout);
static int MX25L12833F_EnableMemoryMappedMode(void);

/* Private functions ---------------------------------------------------------*/
KeepInCompilation HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{ 
  return HAL_OK;
}

uint32_t HAL_GetTick(void)
{
  return 1;
}

/**
  * @brief  System initialization.
  * @param  None
  * @retval  1      : Operation succeeded
  * @retval  0      : Operation failed
  */
int Init()
{
  /* Init structs to Zero */
  char *startadd = __section_begin(".bss");
  uint32_t size = __section_size(".bss");
  memset(startadd, 0, size);
  
  /* Init system */
  SystemInit(); 
  HAL_Init();
  
  /* Configure the system clock */
  if (SystemClock_Config() != 1)
    return 0;
  
  /* Initialize OCTOSPI1 */
  if (MX_OCTOSPI1_Init() != 1)
    return 0;
  
  /* Reset Flash Memory */
  if (MX25L12833F_Reset() != 1)
    return 0;
  
  /* Enable Quad Mode */
  if (MX25L12833F_EnableQuadMode() != 1)
    return 0;
  
  /* Enable Memory-Mapped Mode */
  if (MX25L12833F_EnableMemoryMappedMode() != 1)
    return 0;
  
  return 1;
}

/**
  * @brief  Initialize OCTOSPI1 in Quad SPI mode
  * @retval 1 if success, 0 if failure
  */
static int MX_OCTOSPI1_Init(void)
{
  hospi1.Instance = OCTOSPI1;
  
  /* OCTOSPI1 parameter configuration */
  hospi1.Init.FifoThreshold         = 1;
  hospi1.Init.DualQuad               = HAL_OSPI_DUALQUAD_DISABLE;
  hospi1.Init.MemoryType            = HAL_OSPI_MEMTYPE_MACRONIX;
  hospi1.Init.DeviceSize            = 24; /* 2^(24+1) = 32MB address space (16MB actual) */
  hospi1.Init.ChipSelectHighTime   = 2;
  hospi1.Init.FreeRunningClock     = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi1.Init.ClockMode             = HAL_OSPI_CLOCK_MODE_0;
  hospi1.Init.ClockPrescaler        = 4;  /* 160MHz / 4 = 40MHz */
  hospi1.Init.SampleShifting        = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
  hospi1.Init.ChipSelectBoundary   = 0;
  hospi1.Init.DelayBlockBypass     = HAL_OSPI_DELAY_BLOCK_USED;
  hospi1.Init.MaxTran               = 0;
  hospi1.Init.Refresh               = 0;
  
  if (HAL_OSPI_Init(&hospi1) != HAL_OK)
  {
    return 0;
  }
  
  return 1;
}

/**
  * @brief  Reset the MX25L12833F memory
  * @retval 1 if success, 0 if failure
  */
static int MX25L12833F_Reset(void)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  
  /* Enable Reset */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = RESET_ENABLE_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
  
  if (HAL_OSPI_Command(&hospi1, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return 0;
  }
  
  /* Reset Memory */
  sCommand.Instruction = RESET_MEMORY_CMD;
  
  if (HAL_OSPI_Command(&hospi1, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return 0;
  }
  
  /* Wait for reset completion */
  HAL_Delay(1);
  
  return 1;
}

/**
  * @brief  Enable Quad mode for MX25L12833F
  * @retval 1 if success, 0 if failure
  */
static int MX25L12833F_EnableQuadMode(void)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  uint8_t reg;
  
  /* Read Status Register */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = READ_STATUS_REG_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData             = 1;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
  
  if (HAL_OSPI_Command(&hospi1, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return 0;
  }
  
  if (HAL_OSPI_Receive(&hospi1, &reg, TIMEOUT_VALUE) != HAL_OK)
  {
    return 0;
  }
  
  /* Check if QE bit is already set */
  if ((reg & 0x40) != 0x40)
  {
    /* Enable write operations */
    if (MX25L12833F_WriteEnable() != 1)
    {
      return 0;
    }
    
    /* Set QE bit */
    reg |= 0x40;
    
    /* Write Status Register */
    sCommand.Instruction = WRITE_STATUS_REG_CMD;
    sCommand.DataMode    = HAL_OSPI_DATA_1_LINE;
    
    if (HAL_OSPI_Command(&hospi1, &sCommand, TIMEOUT_VALUE) != HAL_OK)
    {
      return 0;
    }
    
    if (HAL_OSPI_Transmit(&hospi1, &reg, TIMEOUT_VALUE) != HAL_OK)
    {
      return 0;
    }
    
    /* Wait for write completion */
    if (MX25L12833F_WaitForWriteEnd(TIMEOUT_VALUE) != 1)
    {
      return 0;
    }
  }
  
  return 1;
}

/**
  * @brief  Send Write Enable command
  * @retval 1 if success, 0 if failure
  */
static int MX25L12833F_WriteEnable(void)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  
  /* Enable write operations */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = WRITE_ENABLE_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
  
  if (HAL_OSPI_Command(&hospi1, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return 0;
  }
  
  return 1;
}

/**
  * @brief  Wait for write/erase operation completion
  * @param  Timeout: timeout value
  * @retval 1 if success, 0 if failure
  */
static int MX25L12833F_WaitForWriteEnd(uint32_t Timeout)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  uint8_t reg;
  
  /* Configure the command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = READ_STATUS_REG_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData             = 1;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
  
  /* Send the command */
  if (HAL_OSPI_Command(&hospi1, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return 0;
  }
  
  /* Check WIP bit */
  do
  {
    if (HAL_OSPI_Receive(&hospi1, &reg, TIMEOUT_VALUE) != HAL_OK)
    {
      return 0;
    }
    Timeout--;
  } while ((reg & 0x01) && (Timeout > 0));
  
  if (Timeout == 0)
  {
    return 0;
  }
  
  return 1;
}

/**
  * @brief  Enable memory-mapped mode
  * @retval 1 if success, 0 if failure
  */
static int MX25L12833F_EnableMemoryMappedMode(void)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  OSPI_MemoryMappedTypeDef sMemMappedCfg = {0};
  
  /* Configure the command for memory-mapped mode */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_READ_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = QUAD_INOUT_FAST_READ_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytes     = 0x00;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
  sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
  sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
  sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles        = MX25L12833F_DUMMY_CYCLES_READ_QUAD;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
  
  if (HAL_OSPI_Command(&hospi1, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return 0;
  }
  
  /* Configure write command for memory-mapped mode (not used but required) */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_WRITE_CFG;
  sCommand.Instruction        = QUAD_PAGE_PROG_CMD;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DummyCycles        = 0;
  
  if (HAL_OSPI_Command(&hospi1, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return 0;
  }
  
  /* Configure memory-mapped mode */
  sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;
  sMemMappedCfg.TimeOutPeriod     = 0;
  
  if (HAL_OSPI_MemoryMapped(&hospi1, &sMemMappedCfg) != HAL_OK)
  {
    return 0;
  }
  
  return 1;
}

/**
  * @brief  Mass erase of the flash
  * @param  Parallelism: Not used
  * @retval 1 if success, 0 if failure
  */
KeepInCompilation int MassErase(uint32_t Parallelism)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  
  /* Re-initialize OCTOSPI for command mode */
  HAL_OSPI_DeInit(&hospi1);
  MX_OCTOSPI1_Init();
  
  /* Enable write operations */
  if (MX25L12833F_WriteEnable() != 1)
  {
    return 0;
  }
  
  /* Send Chip Erase command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = CHIP_ERASE_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
  
  if (HAL_OSPI_Command(&hospi1, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return 0;
  }
  
  /* Wait for erase completion (max 40 seconds for chip erase) */
  if (MX25L12833F_WaitForWriteEnd(40000) != 1)
  {
    return 0;
  }
  
  return 1;
}

/**
  * @brief  Write data to the flash
  * @param  Address: write address
  * @param  Size: size of data to write
  * @param  buffer: pointer to data buffer
  * @retval 1 if success, 0 if failure
  */
KeepInCompilation int Write(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  uint32_t end_addr, current_size, current_addr;
  uint8_t *write_data;
  
  /* Remove memory-mapped region offset */
  Address = Address & 0x0FFFFFFF;
  
  /* Re-initialize OCTOSPI for command mode */
  HAL_OSPI_DeInit(&hospi1);
  MX_OCTOSPI1_Init();
  
  /* Calculate the size between the write address and the end of the page */
  current_size = MX25L12833F_PAGE_SIZE - (Address % MX25L12833F_PAGE_SIZE);
  
  /* Check if the size of the data is less than the remaining place in the page */
  if (current_size > Size)
  {
    current_size = Size;
  }
  
  /* Initialize the address variables */
  current_addr = Address;
  end_addr = Address + Size;
  write_data = buffer;
  
  /* Configure the write command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = QUAD_PAGE_PROG_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
  
  /* Perform the write page by page */
  do
  {
    /* Enable write operations */
    if (MX25L12833F_WriteEnable() != 1)
    {
      return 0;
    }
    
    /* Set page address and size */
    sCommand.Address = current_addr;
    sCommand.NbData  = current_size;
    
    /* Send the command */
    if (HAL_OSPI_Command(&hospi1, &sCommand, TIMEOUT_VALUE) != HAL_OK)
    {
      return 0;
    }
    
    /* Transmit the data */
    if (HAL_OSPI_Transmit(&hospi1, write_data, TIMEOUT_VALUE) != HAL_OK)
    {
      return 0;
    }
    
    /* Wait for write completion */
    if (MX25L12833F_WaitForWriteEnd(TIMEOUT_VALUE) != 1)
    {
      return 0;
    }
    
    /* Update the address and size variables for next page programming */
    current_addr += current_size;
    write_data += current_size;
    current_size = ((current_addr + MX25L12833F_PAGE_SIZE) > end_addr) ?
                   (end_addr - current_addr) : MX25L12833F_PAGE_SIZE;
  } while (current_addr < end_addr);
  
  return 1;
}

/**
  * @brief  Sector erase (64KB blocks)
  * @param  EraseStartAddress: erase start address
  * @param  EraseEndAddress: erase end address
  * @retval 1 if success, 0 if failure
  */
KeepInCompilation int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  uint32_t BlockAddr;
  
  /* Remove memory-mapped region offset */
  EraseStartAddress &= 0x0FFFFFFF;
  EraseEndAddress &= 0x0FFFFFFF;
  
  /* Align to 64KB block boundary */
  EraseStartAddress = EraseStartAddress - (EraseStartAddress % MX25L12833F_BLOCK_SIZE);
  
  /* Re-initialize OCTOSPI for command mode */
  HAL_OSPI_DeInit(&hospi1);
  MX_OCTOSPI1_Init();
  
  /* Configure the erase command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = BLOCK_ERASE_64K_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
  
  /* Erase block by block */
  while (EraseEndAddress >= EraseStartAddress)
  {
    BlockAddr = EraseStartAddress;
    
    /* Enable write operations */
    if (MX25L12833F_WriteEnable() != 1)
    {
      return 0;
    }
    
    /* Set block address */
    sCommand.Address = BlockAddr;
    
    /* Send the erase command */
    if (HAL_OSPI_Command(&hospi1, &sCommand, TIMEOUT_VALUE) != HAL_OK)
    {
      return 0;
    }
    
    /* Wait for erase completion (typical 150ms, max 2000ms for 64KB block) */
    if (MX25L12833F_WaitForWriteEnd(2000) != 1)
    {
      return 0;
    }
    
    EraseStartAddress += MX25L12833F_BLOCK_SIZE;
  }
  
  return 1;
}

/**
  * @brief  Calculate checksum value of the memory zone
  * @param  StartAddress: Flash start address
  * @param  Size: Size (in WORD)
  * @param  InitVal: Initial CRC value
  * @retval Checksum value
  */
uint32_t CheckSum(uint32_t StartAddress, uint32_t Size, uint32_t InitVal)
{
  uint8_t missalignementAddress = StartAddress % 4;
  uint8_t missalignementSize = Size;
  int cnt;
  uint32_t Val;
  
  StartAddress -= StartAddress % 4;
  Size += (Size % 4 == 0) ? 0 : 4 - (Size % 4);
  
  for(cnt = 0; cnt < Size; cnt += 4)
  {
    Val = *(uint32_t*)StartAddress;
    if(missalignementAddress)
    {
      switch (missalignementAddress)
      {
        case 1:
          InitVal += (uint8_t) (Val >> 8 & 0xff);
          InitVal += (uint8_t) (Val >> 16 & 0xff);
          InitVal += (uint8_t) (Val >> 24 & 0xff);
          missalignementAddress -= 1;
          break;
        case 2:
          InitVal += (uint8_t) (Val >> 16 & 0xff);
          InitVal += (uint8_t) (Val >> 24 & 0xff);
          missalignementAddress -= 2;
          break;
        case 3:
          InitVal += (uint8_t) (Val >> 24 & 0xff);
          missalignementAddress -= 3;
          break;
      }
    }
    else if((Size - missalignementSize) % 4 && (Size - cnt) <= 4)
    {
      switch (Size - missalignementSize)
      {
        case 1:
          InitVal += (uint8_t) Val;
          InitVal += (uint8_t) (Val >> 8 & 0xff);
          InitVal += (uint8_t) (Val >> 16 & 0xff);
          missalignementSize -= 1;
          break;
        case 2:
          InitVal += (uint8_t) Val;
          InitVal += (uint8_t) (Val >> 8 & 0xff);
          missalignementSize -= 2;
          break;
        case 3:
          InitVal += (uint8_t) Val;
          missalignementSize -= 3;
          break;
      }
    }
    else
    {
      InitVal += (uint8_t) Val;
      InitVal += (uint8_t) (Val >> 8 & 0xff);
      InitVal += (uint8_t) (Val >> 16 & 0xff);
      InitVal += (uint8_t) (Val >> 24 & 0xff);
    }
    StartAddress += 4;
  }
  
  return (InitVal);
}

/**
  * @brief  Verify flash memory with RAM buffer
  * @param  MemoryAddr: Flash address
  * @param  RAMBufferAddr: RAM buffer address
  * @param  Size: Size (in WORD)
  * @param  missalignement: missalignement value
  * @retval Operation result (address of failure or checksum)
  */
KeepInCompilation uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr, 
                                    uint32_t Size, uint32_t missalignement)
{
  uint32_t VerifiedData = 0, InitVal = 0;
  uint64_t checksum;
  Size *= 4;
  
  /* Enable memory-mapped mode for verification */
  MX25L12833F_EnableMemoryMappedMode();
  
  checksum = CheckSum((uint32_t)MemoryAddr + (missalignement & 0xf), 
                     Size - ((missalignement >> 16) & 0xF), InitVal);
  
  while (Size > VerifiedData)
  {
    if (*(uint8_t*)MemoryAddr++ != *((uint8_t*)RAMBufferAddr + VerifiedData))
      return ((checksum << 32) + (MemoryAddr + VerifiedData));
    
    VerifiedData++;
  }
  
  return (checksum << 32);
}

/**
  * @brief  System Clock Configuration for STM32U5G9
  *         System Clock source = PLL (MSI)
  *         SYSCLK(Hz) = 160000000
  *         HCLK(Hz)   = 160000000
  * @retval 1 if success, 0 if failure
  */
static int SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* Enable voltage range 1 for frequency above 100 Mhz */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* MSI Oscillator enabled at reset (4Mhz), activate PLL with MSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;  /* 4 MHz */
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 80;  /* 4MHz * 80 / 2 = 160MHz */
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    return 0;
  }

  /* Select PLL as system clock source and configure bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | 
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | 
                                 RCC_CLOCKTYPE_PCLK3);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    return 0;
  }
  
  return 1;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
