/**
  ******************************************************************************
  * @file    mx25l12833f.c
  * @brief   MX25L12833F QSPI Flash Driver Implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "mx25l12833f.h"
#include <string.h>

/* Private define ------------------------------------------------------------*/
#define TIMEOUT_VALUE   5000

/* Private functions ---------------------------------------------------------*/
static MX25L12833F_Status_t MX25L12833F_WritePage(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t WriteAddr, uint32_t Size);

/**
  * @brief  Initialize the MX25L12833F flash
  * @param  hospi: OSPI handle
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_Init(OSPI_HandleTypeDef *hospi)
{
  /* Reset the flash */
  if (MX25L12833F_Reset(hospi) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Enable Quad mode */
  if (MX25L12833F_EnableQuadMode(hospi) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Reset the MX25L12833F flash
  * @param  hospi: OSPI handle
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_Reset(OSPI_HandleTypeDef *hospi)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Initialize the reset enable command */
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

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Initialize the reset memory command */
  sCommand.Instruction = RESET_MEMORY_CMD;

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Wait for reset completion (tRST = 30us worst case) */
  HAL_Delay(1);

  return MX25L12833F_OK;
}

/**
  * @brief  Get flash information
  * @param  pInfo: pointer to information structure
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_GetInfo(MX25L12833F_Info_t *pInfo)
{
  pInfo->FlashSize       = MX25L12833F_FLASH_SIZE;
  pInfo->SectorSize      = MX25L12833F_SECTOR_SIZE;
  pInfo->SectorsNumber   = (MX25L12833F_FLASH_SIZE / MX25L12833F_SECTOR_SIZE);
  pInfo->PageSize        = MX25L12833F_PAGE_SIZE;
  pInfo->PagesNumber     = (MX25L12833F_FLASH_SIZE / MX25L12833F_PAGE_SIZE);

  return MX25L12833F_OK;
}

/**
  * @brief  Enable Quad mode for MX25L12833F
  * @param  hospi: OSPI handle
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_EnableQuadMode(OSPI_HandleTypeDef *hospi)
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

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  if (HAL_OSPI_Receive(hospi, &reg, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Check if QE bit is already set */
  if ((reg & MX25L12833F_SR_QE) != 0)
  {
    return MX25L12833F_OK;
  }

  /* Enable write operations */
  if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Set QE bit */
  reg |= MX25L12833F_SR_QE;

  sCommand.Instruction = WRITE_STATUS_REG_CMD;
  sCommand.NbData      = 1;

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  if (HAL_OSPI_Transmit(hospi, &reg, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Wait for write completion */
  if (MX25L12833F_WaitForWriteEnd(hospi, MX25L12833F_WRITE_REG_MAX_TIME) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Disable Quad mode for MX25L12833F
  * @param  hospi: OSPI handle
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_DisableQuadMode(OSPI_HandleTypeDef *hospi)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  uint8_t reg;

  /* Read Status Register */
  if (MX25L12833F_ReadStatusRegister(hospi, &reg) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Check if QE bit is already cleared */
  if ((reg & MX25L12833F_SR_QE) == 0)
  {
    return MX25L12833F_OK;
  }

  /* Enable write operations */
  if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Clear QE bit */
  reg &= ~MX25L12833F_SR_QE;

  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = WRITE_STATUS_REG_CMD;
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

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  if (HAL_OSPI_Transmit(hospi, &reg, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Wait for write completion */
  if (MX25L12833F_WaitForWriteEnd(hospi, MX25L12833F_WRITE_REG_MAX_TIME) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Read data from the flash
  * @param  hospi: OSPI handle
  * @param  pData: pointer to data buffer
  * @param  ReadAddr: read start address
  * @param  Size: size of data to read
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_Read(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t ReadAddr, uint32_t Size)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Initialize the read command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = QUAD_INOUT_FAST_READ_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Address            = ReadAddr;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytes     = 0x00;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
  sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
  sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
  sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData             = Size;
  sCommand.DummyCycles        = MX25L12833F_DUMMY_CYCLES_READ_QUAD;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  if (HAL_OSPI_Receive(hospi, pData, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Write data to the flash
  * @param  hospi: OSPI handle
  * @param  pData: pointer to data buffer
  * @param  WriteAddr: write start address
  * @param  Size: size of data to write
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_Write(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t WriteAddr, uint32_t Size)
{
  uint32_t end_addr, current_size, current_addr;
  uint8_t *write_data;

  /* Calculation of the size between the write address and the end of the page */
  current_size = MX25L12833F_PAGE_SIZE - (WriteAddr % MX25L12833F_PAGE_SIZE);

  /* Check if the size of the data is less than the remaining place in the page */
  if (current_size > Size)
  {
    current_size = Size;
  }

  /* Initialize the address variables */
  current_addr = WriteAddr;
  end_addr = WriteAddr + Size;
  write_data = pData;

  /* Perform the write page by page */
  do
  {
    /* Write data to the flash */
    if (MX25L12833F_WritePage(hospi, write_data, current_addr, current_size) != MX25L12833F_OK)
    {
      return MX25L12833F_ERROR;
    }

    /* Update the address and size variables for next page programming */
    current_addr += current_size;
    write_data += current_size;
    current_size = ((current_addr + MX25L12833F_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : MX25L12833F_PAGE_SIZE;
  } while (current_addr < end_addr);

  return MX25L12833F_OK;
}

/**
  * @brief  Write a page to the flash
  * @param  hospi: OSPI handle
  * @param  pData: pointer to data buffer
  * @param  WriteAddr: write start address
  * @param  Size: size of data to write (max 256 bytes)
  * @retval MX25L12833F status
  */
static MX25L12833F_Status_t MX25L12833F_WritePage(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t WriteAddr, uint32_t Size)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Enable write operations */
  if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Initialize the program command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = QUAD_PAGE_PROG_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Address            = WriteAddr;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData             = Size;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  if (HAL_OSPI_Transmit(hospi, pData, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Wait for write completion */
  if (MX25L12833F_WaitForWriteEnd(hospi, MX25L12833F_PAGE_PROG_MAX_TIME) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Erase a 4KB sector
  * @param  hospi: OSPI handle
  * @param  SectorAddress: sector address to erase
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_Erase_Sector(OSPI_HandleTypeDef *hospi, uint32_t SectorAddress)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Enable write operations */
  if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Initialize the erase command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = SECTOR_ERASE_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Address            = SectorAddress;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Wait for erase completion */
  if (MX25L12833F_WaitForWriteEnd(hospi, MX25L12833F_SECTOR_ERASE_MAX_TIME) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Erase a 64KB block
  * @param  hospi: OSPI handle
  * @param  BlockAddress: block address to erase
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_Erase_Block(OSPI_HandleTypeDef *hospi, uint32_t BlockAddress)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Enable write operations */
  if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Initialize the erase command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = BLOCK_ERASE_64K_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Address            = BlockAddress;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Wait for erase completion */
  if (MX25L12833F_WaitForWriteEnd(hospi, MX25L12833F_BLOCK_ERASE_64K_MAX_TIME) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Erase the whole flash chip
  * @param  hospi: OSPI handle
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_Erase_Chip(OSPI_HandleTypeDef *hospi)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Enable write operations */
  if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Initialize the erase command */
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

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Wait for erase completion (max 40 seconds) */
  if (MX25L12833F_WaitForWriteEnd(hospi, MX25L12833F_CHIP_ERASE_MAX_TIME) != MX25L12833F_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Enable memory-mapped mode
  * @param  hospi: OSPI handle
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_EnableMemoryMappedMode(OSPI_HandleTypeDef *hospi)
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

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  /* Configure the memory-mapped mode */
  sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_ENABLE;
  sMemMappedCfg.TimeOutPeriod     = 0x20;

  if (HAL_OSPI_MemoryMapped(hospi, &sMemMappedCfg) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Enable write operations
  * @param  hospi: OSPI handle
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_WriteEnable(OSPI_HandleTypeDef *hospi)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Initialize the command */
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

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Disable write operations
  * @param  hospi: OSPI handle
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_WriteDisable(OSPI_HandleTypeDef *hospi)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Initialize the command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = WRITE_DISABLE_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Read status register
  * @param  hospi: OSPI handle
  * @param  pData: pointer to status register value
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_ReadStatusRegister(OSPI_HandleTypeDef *hospi, uint8_t *pData)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Initialize the command */
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

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  if (HAL_OSPI_Receive(hospi, pData, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}

/**
  * @brief  Wait for write/erase operation to complete
  * @param  hospi: OSPI handle
  * @param  Timeout: timeout in milliseconds
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_WaitForWriteEnd(OSPI_HandleTypeDef *hospi, uint32_t Timeout)
{
  uint8_t status;
  uint32_t tickstart = HAL_GetTick();

  /* Wait for the write/erase operation to complete */
  do
  {
    /* Read the status register */
    if (MX25L12833F_ReadStatusRegister(hospi, &status) != MX25L12833F_OK)
    {
      return MX25L12833F_ERROR;
    }

    /* Check for timeout */
    if ((HAL_GetTick() - tickstart) > Timeout)
    {
      return MX25L12833F_TIMEOUT;
    }

  } while ((status & MX25L12833F_SR_WIP) != 0);

  return MX25L12833F_OK;
}

/**
  * @brief  Read flash ID
  * @param  hospi: OSPI handle
  * @param  pData: pointer to ID data (3 bytes: Manufacturer ID, Memory Type, Capacity)
  * @retval MX25L12833F status
  */
MX25L12833F_Status_t MX25L12833F_ReadID(OSPI_HandleTypeDef *hospi, uint8_t *pData)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Initialize the command */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = READ_ID_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData             = 3;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(hospi, &sCommand, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  if (HAL_OSPI_Receive(hospi, pData, TIMEOUT_VALUE) != HAL_OK)
  {
    return MX25L12833F_ERROR;
  }

  return MX25L12833F_OK;
}
