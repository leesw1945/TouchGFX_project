/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @author  MCD Application Team
  * @brief   This file implements the External Loader functions for
  *          MX25L12833F Quad-SPI memory.
  ******************************************************************************
  */

#include "Loader_Src.h"
#include "string.h"

/* Private variables ---------------------------------------------------------*/
OSPI_HandleTypeDef hospi1;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_OCTOSPI1_Init(void);
static HAL_StatusTypeDef OSPI_WriteEnable(OSPI_HandleTypeDef *hospi);
static HAL_StatusTypeDef OSPI_AutoPollingMemReady(OSPI_HandleTypeDef *hospi, uint32_t Timeout);
static HAL_StatusTypeDef OSPI_QuadMode(OSPI_HandleTypeDef *hospi, uint8_t enable);
static HAL_StatusTypeDef OSPI_ResetMemory(OSPI_HandleTypeDef *hospi);
static HAL_StatusTypeDef OSPI_MemoryMappedMode(OSPI_HandleTypeDef *hospi);

/**
  * @brief  System initialization.
  * @param  None
  * @retval  1      : Operation succeeded
  * @retval  0      : Operation failed
  */
KeepInCompilation int Init(void)
{
  /* Reset of all peripherals */
  HAL_Init();

  /* System Clock Configuration */
  SystemClock_Config();

  /* GPIO Initialization */
  MX_GPIO_Init();

  /* OCTOSPI1 Initialization */
  MX_OCTOSPI1_Init();

  /* Reset the memory */
  if (OSPI_ResetMemory(&hospi1) != HAL_OK)
  {
    return 0;
  }

  /* Enable Quad mode */
  if (OSPI_QuadMode(&hospi1, 1) != HAL_OK)
  {
    return 0;
  }

  /* Configure memory mapped mode */
  if (OSPI_MemoryMappedMode(&hospi1) != HAL_OK)
  {
    return 0;
  }

  return 1;
}

/**
  * @brief  Program memory.
  * @param  Address: page address
  * @param  Size   : size of data
  * @param  buffer : pointer to data buffer
  * @retval  1      : Operation succeeded
  * @retval  0      : Operation failed
  */
KeepInCompilation int Write(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
  uint32_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
  uint32_t   OSPI_DataSize = 0;
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Calculation of the size between the write address and the end of the page */
  temp = MEMORY_PAGE_SIZE - (Address % MEMORY_PAGE_SIZE);

  /* Check if the size of the data is less than the remaining place in the page */
  if (temp >= Size)
  {
    OSPI_DataSize = Size;
  }
  else
  {
    OSPI_DataSize = temp;
  }

  /* Initialize the address variables */
  Addr = Address;

  /* Calculate the number of pages to write */
  NumOfPage =  Size / MEMORY_PAGE_SIZE;
  NumOfSingle = Size % MEMORY_PAGE_SIZE;

  if (NumOfSingle == 0)
  {
    NumOfSingle = MEMORY_PAGE_SIZE;
    NumOfPage--;
  }

  /* Perform the write page by page */
  do
  {
    /* Check if the amount of data remaining is smaller than the page size */
    if (count != 0)
    {
      OSPI_DataSize = (count <= MEMORY_PAGE_SIZE) ? count : MEMORY_PAGE_SIZE;
    }

    /* Enable write operations */
    if (OSPI_WriteEnable(&hospi1) != HAL_OK)
    {
      return 0;
    }

    /* Configure the command for page program */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = QUAD_PAGE_PROG_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = Addr;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData             = OSPI_DataSize;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return 0;
    }

    /* Transmission of the data */
    if (HAL_OSPI_Transmit(&hospi1, buffer, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return 0;
    }

    /* Wait for the end of the transfer */
    if (OSPI_AutoPollingMemReady(&hospi1, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return 0;
    }

    /* Update the address, size and buffer pointer for the next page program */
    Addr += OSPI_DataSize;
    buffer += OSPI_DataSize;
    count = (count < OSPI_DataSize) ? 0 : (count - OSPI_DataSize);

  } while ((count != 0) || (NumOfPage-- != 0));

  return 1;
}

/**
  * @brief  Sector erase.
  * @param  EraseStartAddress : Start of sector
  * @param  EraseEndAddress   : End of sector
  * @retval  1      : Operation succeeded
  * @retval  0      : Operation failed
  */
KeepInCompilation int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
  uint32_t BlockAddr;
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Erase block by block */
  EraseStartAddress = EraseStartAddress - (EraseStartAddress % MEMORY_BLOCK_SIZE);

  while (EraseEndAddress >= EraseStartAddress)
  {
    BlockAddr = EraseStartAddress;

    /* Enable write operations */
    if (OSPI_WriteEnable(&hospi1) != HAL_OK)
    {
      return 0;
    }

    /* Configure the command for block erase (64KB) */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = BLOCK_ERASE_64K_CMD;
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

    if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return 0;
    }

    /* Wait for the end of the erase operation */
    if (OSPI_AutoPollingMemReady(&hospi1, 400000) != HAL_OK)
    {
      return 0;
    }

    EraseStartAddress += MEMORY_BLOCK_SIZE;
  }

  return 1;
}

/**
  * @brief  Mass erase of the entire chip.
  * @param  Parallelism : 0
  * @retval  1           : Operation succeeded
  * @retval  0           : Operation failed
  */
KeepInCompilation int MassErase(uint32_t Parallelism)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Enable write operations */
  if (OSPI_WriteEnable(&hospi1) != HAL_OK)
  {
    return 0;
  }

  /* Configure the command for chip erase */
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

  if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return 0;
  }

  /* Wait for the end of the erase operation (can take several seconds) */
  if (OSPI_AutoPollingMemReady(&hospi1, 400000) != HAL_OK)
  {
    return 0;
  }

  return 1;
}

/**
  * @brief  Verify flash memory.
  * @param  MemoryAddr    : Memory address
  * @param  RAMBufferAddr : RAM buffer address
  * @param  Size          : Size in bytes
  * @param  missalignement: Alignment
  * @retval  Checksum value
  */
KeepInCompilation uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr, 
                                   uint32_t Size, uint32_t missalignement)
{
  uint32_t VerifiedData = 0;
  uint8_t TmpBuffer = 0x00;
  uint64_t checksum = 0;
  Size *= 4;

  checksum = CheckSum((uint32_t)MemoryAddr + (missalignement & 0xF), Size, missalignement);

  while (Size > VerifiedData)
  {
    TmpBuffer = *(uint8_t*)(MemoryAddr + VerifiedData + missalignement);

    if (TmpBuffer != *((uint8_t*)RAMBufferAddr + VerifiedData))
    {
      return ((checksum << 32) + (MemoryAddr + VerifiedData));
    }

    VerifiedData++;
  }

  return (checksum << 32);
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  System Clock Configuration
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* Configure the main internal regulator output voltage */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    while(1);
  }

  /* Initializes the CPU, AHB and APB busses clocks */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;  /* 4 MHz */
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_0;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    while(1);
  }

  /* Initializes the CPU, AHB and APB busses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    while(1);
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
}

/**
  * @brief OCTOSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_OCTOSPI1_Init(void)
{
  OSPIM_CfgTypeDef sOspiManagerCfg = {0};

  /* OCTOSPI1 parameter configuration*/
  hospi1.Instance = OCTOSPI1;
  hospi1.Init.FifoThreshold = 4;
  hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
  hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
  hospi1.Init.DeviceSize = 24;  /* 2^24 = 16MB */
  hospi1.Init.ChipSelectHighTime = 2;
  hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
  hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
  hospi1.Init.ClockPrescaler = 2;
  hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
  hospi1.Init.ChipSelectBoundary = 0;
  hospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
  hospi1.Init.MaxTran = 0;
  hospi1.Init.Refresh = 0;

  if (HAL_OSPI_Init(&hospi1) != HAL_OK)
  {
    while(1);
  }

  sOspiManagerCfg.ClkPort = 1;
  sOspiManagerCfg.DQSPort = 1;
  sOspiManagerCfg.NCSPort = 1;
  sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
  sOspiManagerCfg.IOHighPort = HAL_OSPIM_IOPORT_1_HIGH;

  if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    while(1);
  }
}

/**
  * @brief  This function sends a Write Enable command
  * @param  hospi: OSPI handle
  * @retval HAL status
  */
static HAL_StatusTypeDef OSPI_WriteEnable(OSPI_HandleTypeDef *hospi)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  OSPI_AutoPollingTypeDef sConfig = {0};

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

  if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Configure automatic polling mode to wait for write enable */
  sCommand.Instruction    = READ_STATUS_REG_CMD;
  sCommand.DataMode       = HAL_OSPI_DATA_1_LINE;
  sCommand.NbData         = 1;
  sCommand.DataDtrMode    = HAL_OSPI_DATA_DTR_DISABLE;

  sConfig.Match           = STATUS_REG_WEL_MASK;
  sConfig.Mask            = STATUS_REG_WEL_MASK;
  sConfig.MatchMode       = HAL_OSPI_MATCH_MODE_AND;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_OSPI_AutoPolling(hospi, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  This function waits for the end of the ongoing operation
  * @param  hospi: OSPI handle
  * @param  Timeout: Timeout for auto-polling
  * @retval HAL status
  */
static HAL_StatusTypeDef OSPI_AutoPollingMemReady(OSPI_HandleTypeDef *hospi, uint32_t Timeout)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  OSPI_AutoPollingTypeDef sConfig = {0};

  /* Configure automatic polling mode to wait for memory ready */
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

  sConfig.Match           = 0;
  sConfig.Mask            = STATUS_REG_WIP_MASK;
  sConfig.MatchMode       = HAL_OSPI_MATCH_MODE_AND;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_OSPI_AutoPolling(hospi, &sConfig, Timeout) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  Enable/Disable Quad mode
  * @param  hospi: OSPI handle
  * @param  enable: 1 = enable, 0 = disable
  * @retval HAL status
  */
static HAL_StatusTypeDef OSPI_QuadMode(OSPI_HandleTypeDef *hospi, uint8_t enable)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  uint8_t reg[2];

  /* Read status and configuration register */
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

  if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Receive(hospi, &reg[0], HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Read configuration register */
  sCommand.Instruction = READ_CFG_REG_CMD;

  if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Receive(hospi, &reg[1], HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Enable/Disable Quad mode */
  if (enable)
  {
    reg[1] |= CFG_REG_QUAD_MASK;  /* Set Quad Enable bit */
  }
  else
  {
    reg[1] &= ~CFG_REG_QUAD_MASK; /* Clear Quad Enable bit */
  }

  /* Enable write operations */
  if (OSPI_WriteEnable(hospi) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Write status and configuration register */
  sCommand.Instruction = WRITE_STATUS_CFG_REG_CMD;
  sCommand.NbData      = 2;

  if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Transmit(hospi, reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Wait for write operation to complete */
  if (OSPI_AutoPollingMemReady(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  Reset the memory
  * @param  hospi: OSPI handle
  * @retval HAL status
  */
static HAL_StatusTypeDef OSPI_ResetMemory(OSPI_HandleTypeDef *hospi)
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

  if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Reset Device */
  sCommand.Instruction = RESET_MEMORY_CMD;

  if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Wait for reset to complete */
  HAL_Delay(1);

  return HAL_OK;
}

/**
  * @brief  Configure memory mapped mode
  * @param  hospi: OSPI handle
  * @retval HAL status
  */
static HAL_StatusTypeDef OSPI_MemoryMappedMode(OSPI_HandleTypeDef *hospi)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  OSPI_MemoryMappedTypeDef sMemMappedCfg = {0};

  /* Configure the command for read operation in quad mode */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_READ_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = QUAD_INOUT_READ_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Address            = 0;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
  sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
  sCommand.AlternateBytes     = 0;
  sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
  sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData             = 1;
  sCommand.DummyCycles        = DUMMY_CYCLES_READ_QUAD;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Configure memory mapped mode */
  sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;

  if (HAL_OSPI_MemoryMapped(hospi, &sMemMappedCfg) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief OCTOSPI MSP Initialization
  * @param hospi: OSPI handle pointer
  * @retval None
  */
void HAL_OSPI_MspInit(OSPI_HandleTypeDef* hospi)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if(hospi->Instance == OCTOSPI1)
  {
    /* Peripheral clock enable */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
    PeriphClkInit.OspiClockSelection = RCC_OSPICLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      while(1);
    }

    __HAL_RCC_OSPIM_CLK_ENABLE();
    __HAL_RCC_OSPI1_CLK_ENABLE();

    /**
     * OCTOSPI1 GPIO Configuration
     * 
     * NOTE: These pin assignments are EXAMPLES.
     * You MUST modify these to match your actual PCB design!
     * 
     * Common configurations:
     * - STM32U5G9ZJT6Q supports multiple pin options for OCTOSPI
     * - Check your schematic for actual connections
     */
    
    /* Example pin configuration - MODIFY TO MATCH YOUR HARDWARE */
    
    /* OCTOSPI1_CLK: PB10 (AF10) - EXAMPLE, verify your schematic! */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* OCTOSPI1_NCS: PB11 (AF10) - EXAMPLE, verify your schematic! */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* OCTOSPI1_IO0: PC9 (AF10) - EXAMPLE, verify your schematic! */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* OCTOSPI1_IO1: PC10 (AF10) - EXAMPLE, verify your schematic! */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* OCTOSPI1_IO2: PE2 (AF10) - EXAMPLE, verify your schematic! */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* OCTOSPI1_IO3: PA1 (AF10) - EXAMPLE, verify your schematic! */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}

/**
  * @brief OCTOSPI MSP De-Initialization
  * @param hospi: OSPI handle pointer
  * @retval None
  */
void HAL_OSPI_MspDeInit(OSPI_HandleTypeDef* hospi)
{
  if(hospi->Instance == OCTOSPI1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_OSPI1_CLK_DISABLE();
    __HAL_RCC_OSPIM_CLK_DISABLE();

    /* GPIO de-configuration - MODIFY TO MATCH YOUR HARDWARE */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10 | GPIO_PIN_11);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_9 | GPIO_PIN_10);
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_2);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1);
  }
}
