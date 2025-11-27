/**
  ******************************************************************************
  * @file    mx25l12833f.c
  * @brief   MX25L12833F Quad SPI NOR Flash Driver Implementation
  *          16MB (128Mbit) Serial Multi I/O Flash Memory
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "mx25l12833f.h"

/* Private defines -----------------------------------------------------------*/
#define MX25L12833F_TIMEOUT_DEFAULT_VALUE   5000U   /* 5 seconds */
#define MX25L12833F_AUTOPOLLING_INTERVAL    0x10

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Reset the flash memory
  * @param  hospi: OSPI handle
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_ResetMemory(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Initialize the reset enable command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_RESET_ENABLE_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    /* Send Reset Enable command */
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Send Reset command */
    sCommand.Instruction = MX25L12833F_RESET_CMD;
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Wait for recovery time (tRST) */
    HAL_Delay(1);
    
    return MX25L12833F_OK;
}

/**
  * @brief  Initialize the MX25L12833F flash memory
  * @param  hospi: OSPI handle
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_Init(OSPI_HandleTypeDef *hospi)
{
    /* Reset the memory */
    if (MX25L12833F_ResetMemory(hospi) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Wait for memory ready */
    if (MX25L12833F_AutoPollingMemReady(hospi, MX25L12833F_TIMEOUT_DEFAULT_VALUE) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Enable Quad mode */
    if (MX25L12833F_QuadEnable(hospi) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  De-initialize the MX25L12833F flash memory
  * @param  hospi: OSPI handle
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_DeInit(OSPI_HandleTypeDef *hospi)
{
    /* Reset the memory */
    return MX25L12833F_ResetMemory(hospi);
}

/**
  * @brief  Read Flash ID (JEDEC ID)
  * @param  hospi: OSPI handle
  * @param  id: Buffer to store ID (3 bytes: Manufacturer, Type, Density)
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_ReadID(OSPI_HandleTypeDef *hospi, uint8_t *id)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Initialize the read ID command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_READ_ID_CMD;
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
    
    /* Configure and receive data */
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    if (HAL_OSPI_Receive(hospi, id, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Enable Write operation
  * @param  hospi: OSPI handle
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_WriteEnable(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_AutoPollingTypeDef sConfig = {0};
    
    /* Initialize the write enable command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_WRITE_ENABLE_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    /* Send the command */
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Configure automatic polling to wait for write enable bit */
    sCommand.Instruction  = MX25L12833F_READ_STATUS_REG_CMD;
    sCommand.DataMode     = HAL_OSPI_DATA_1_LINE;
    sCommand.DataDtrMode  = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData       = 1;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    sConfig.Match         = MX25L12833F_SR_WEL;
    sConfig.Mask          = MX25L12833F_SR_WEL;
    sConfig.MatchMode     = HAL_OSPI_MATCH_MODE_AND;
    sConfig.Interval      = MX25L12833F_AUTOPOLLING_INTERVAL;
    sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;
    
    if (HAL_OSPI_AutoPolling(hospi, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Disable Write operation
  * @param  hospi: OSPI handle
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_WriteDisable(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Initialize the write disable command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_WRITE_DISABLE_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    /* Send the command */
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Auto-polling for memory ready status
  * @param  hospi: OSPI handle
  * @param  Timeout: Timeout value in ms
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_AutoPollingMemReady(OSPI_HandleTypeDef *hospi, uint32_t Timeout)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_AutoPollingTypeDef sConfig = {0};
    
    /* Configure automatic polling mode to wait for memory ready */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_READ_STATUS_REG_CMD;
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
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    sConfig.Match         = 0x00;
    sConfig.Mask          = MX25L12833F_SR_WIP;
    sConfig.MatchMode     = HAL_OSPI_MATCH_MODE_AND;
    sConfig.Interval      = MX25L12833F_AUTOPOLLING_INTERVAL;
    sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;
    
    if (HAL_OSPI_AutoPolling(hospi, &sConfig, Timeout) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Read Status Register
  * @param  hospi: OSPI handle
  * @param  status: Pointer to store status value
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_ReadStatusRegister(OSPI_HandleTypeDef *hospi, uint8_t *status)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Initialize the read status register command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_READ_STATUS_REG_CMD;
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
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    if (HAL_OSPI_Receive(hospi, status, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Enable Quad mode
  * @param  hospi: OSPI handle
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_QuadEnable(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint8_t regValue[2] = {0};
    
    /* Read current status register */
    if (MX25L12833F_ReadStatusRegister(hospi, &regValue[0]) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Check if QE bit is already set */
    if ((regValue[0] & MX25L12833F_SR_QE) != 0)
    {
        return MX25L12833F_OK;  /* Already enabled */
    }
    
    /* Set QE bit */
    regValue[0] |= MX25L12833F_SR_QE;
    regValue[1] = 0x00;  /* Configuration register value */
    
    /* Enable write */
    if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Write status register */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_WRITE_STATUS_REG_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData             = 2;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    if (HAL_OSPI_Transmit(hospi, regValue, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Wait for write completion */
    if (MX25L12833F_AutoPollingMemReady(hospi, MX25L12833F_TIMEOUT_DEFAULT_VALUE) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Read data from flash (Standard SPI mode)
  * @param  hospi: OSPI handle
  * @param  pData: Pointer to data buffer
  * @param  ReadAddr: Read start address
  * @param  Size: Size of data to read
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_Read(OSPI_HandleTypeDef *hospi, uint8_t *pData, 
                                            uint32_t ReadAddr, uint32_t Size)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Initialize the read command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_READ_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = ReadAddr;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData             = Size;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    if (HAL_OSPI_Receive(hospi, pData, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Fast Read data from flash
  * @param  hospi: OSPI handle
  * @param  pData: Pointer to data buffer
  * @param  ReadAddr: Read start address
  * @param  Size: Size of data to read
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_FastRead(OSPI_HandleTypeDef *hospi, uint8_t *pData, 
                                                uint32_t ReadAddr, uint32_t Size)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Initialize the fast read command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_FAST_READ_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = ReadAddr;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData             = Size;
    sCommand.DummyCycles        = MX25L12833F_DUMMY_CYCLES_FAST_READ;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    if (HAL_OSPI_Receive(hospi, pData, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Quad Read data from flash (4READ - 0xEB)
  * @param  hospi: OSPI handle
  * @param  pData: Pointer to data buffer
  * @param  ReadAddr: Read start address
  * @param  Size: Size of data to read
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_QuadRead(OSPI_HandleTypeDef *hospi, uint8_t *pData, 
                                                uint32_t ReadAddr, uint32_t Size)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Initialize the quad read command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_QUAD_INOUT_READ_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = ReadAddr;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData             = Size;
    sCommand.DummyCycles        = MX25L12833F_DUMMY_CYCLES_QUAD_READ;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    if (HAL_OSPI_Receive(hospi, pData, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Page Program (max 256 bytes)
  * @param  hospi: OSPI handle
  * @param  pData: Pointer to data buffer
  * @param  WriteAddr: Write start address
  * @param  Size: Size of data to write (max 256 bytes)
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_PageProgram(OSPI_HandleTypeDef *hospi, uint8_t *pData, 
                                                   uint32_t WriteAddr, uint32_t Size)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Enable write */
    if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Initialize the page program command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_PAGE_PROG_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = WriteAddr;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData             = Size;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    if (HAL_OSPI_Transmit(hospi, pData, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Wait for programming to complete */
    if (MX25L12833F_AutoPollingMemReady(hospi, MX25L12833F_PAGE_PROGRAM_TIME / 1000 + 10) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Quad Page Program (4PP - 0x38)
  * @param  hospi: OSPI handle
  * @param  pData: Pointer to data buffer
  * @param  WriteAddr: Write start address
  * @param  Size: Size of data to write (max 256 bytes)
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_QuadPageProgram(OSPI_HandleTypeDef *hospi, uint8_t *pData, 
                                                       uint32_t WriteAddr, uint32_t Size)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Enable write */
    if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Initialize the quad page program command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_QUAD_PAGE_PROG_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = WriteAddr;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData             = Size;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    if (HAL_OSPI_Transmit(hospi, pData, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Wait for programming to complete */
    if (MX25L12833F_AutoPollingMemReady(hospi, MX25L12833F_PAGE_PROGRAM_TIME / 1000 + 10) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Write data to flash (handles page boundaries)
  * @param  hospi: OSPI handle
  * @param  pData: Pointer to data buffer
  * @param  WriteAddr: Write start address
  * @param  Size: Size of data to write
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_Write(OSPI_HandleTypeDef *hospi, uint8_t *pData, 
                                             uint32_t WriteAddr, uint32_t Size)
{
    uint32_t currentAddr = WriteAddr;
    uint32_t endAddr = WriteAddr + Size;
    uint32_t currentSize;
    uint8_t *currentData = pData;
    
    while (currentAddr < endAddr)
    {
        /* Calculate bytes to write in current page */
        currentSize = MX25L12833F_PAGE_SIZE - (currentAddr % MX25L12833F_PAGE_SIZE);
        if (currentSize > (endAddr - currentAddr))
        {
            currentSize = endAddr - currentAddr;
        }
        
        /* Write page */
        if (MX25L12833F_PageProgram(hospi, currentData, currentAddr, currentSize) != MX25L12833F_OK)
        {
            return MX25L12833F_ERROR;
        }
        
        currentAddr += currentSize;
        currentData += currentSize;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Erase a 4KB sector
  * @param  hospi: OSPI handle
  * @param  SectorAddr: Address within the sector to erase
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_Erase_Sector(OSPI_HandleTypeDef *hospi, uint32_t SectorAddr)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Enable write */
    if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Initialize the sector erase command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_SECTOR_ERASE_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = SectorAddr;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Wait for erase to complete */
    if (MX25L12833F_AutoPollingMemReady(hospi, MX25L12833F_SECTOR_ERASE_TIME / 1000 + 100) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Erase a 32KB block
  * @param  hospi: OSPI handle
  * @param  BlockAddr: Address within the block to erase
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_Erase_Block32K(OSPI_HandleTypeDef *hospi, uint32_t BlockAddr)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Enable write */
    if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Initialize the block erase command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_BLOCK_ERASE_32K_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = BlockAddr;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Wait for erase to complete */
    if (MX25L12833F_AutoPollingMemReady(hospi, MX25L12833F_BLOCK_ERASE_TIME / 1000 + 100) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Erase a 64KB block
  * @param  hospi: OSPI handle
  * @param  BlockAddr: Address within the block to erase
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_Erase_Block64K(OSPI_HandleTypeDef *hospi, uint32_t BlockAddr)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Enable write */
    if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Initialize the block erase command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_BLOCK_ERASE_64K_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = BlockAddr;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Wait for erase to complete */
    if (MX25L12833F_AutoPollingMemReady(hospi, MX25L12833F_BLOCK_ERASE_TIME / 1000 + 100) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Erase entire chip
  * @param  hospi: OSPI handle
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_Erase_Chip(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Enable write */
    if (MX25L12833F_WriteEnable(hospi) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Initialize the chip erase command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_CHIP_ERASE_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Wait for erase to complete (very long!) */
    if (MX25L12833F_AutoPollingMemReady(hospi, MX25L12833F_CHIP_ERASE_TIME / 1000 + 1000) != MX25L12833F_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Enable Memory Mapped Mode
  * @param  hospi: OSPI handle
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_EnableMemoryMappedMode(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_MemoryMappedTypeDef sMemMappedCfg = {0};
    
    /* Configure the command for Quad Read (4READ) */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_READ_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = MX25L12833F_QUAD_INOUT_READ_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.DummyCycles        = MX25L12833F_DUMMY_CYCLES_QUAD_READ;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    /* Configure memory mapped mode */
    sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;
    
    if (HAL_OSPI_MemoryMapped(hospi, &sMemMappedCfg) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}

/**
  * @brief  Disable Memory Mapped Mode (Abort operation)
  * @param  hospi: OSPI handle
  * @retval Status
  */
MX25L12833F_StatusTypeDef MX25L12833F_DisableMemoryMappedMode(OSPI_HandleTypeDef *hospi)
{
    if (HAL_OSPI_Abort(hospi) != HAL_OK)
    {
        return MX25L12833F_ERROR;
    }
    
    return MX25L12833F_OK;
}
