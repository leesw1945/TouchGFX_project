/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @brief   External Loader for MX25L12833F Quad-SPI memory
  * 
  * @note    핵심 수정사항:
  *          1. BSS 수동 초기화
  *          2. PLL 없이 HSI 직접 사용 (안정성 향상)
  *          3. OSPI 클럭을 MSI로 설정 (PLL 의존성 제거)
  *          4. 모든 HAL 함수 반환값 체크
  ******************************************************************************
  */

#include "Loader_Src.h"
#include "string.h"
#include "octospi.h"

#define KeepInCompilation __attribute__((used))

/* External symbols from linker script for BSS initialization */
extern uint32_t _sbss;
extern uint32_t _ebss;

/* Private function prototypes -----------------------------------------------*/
static void InitBSS(void);
static int SystemClock_Config(void);
static void MX_GPIO_Init(void);
static HAL_StatusTypeDef OSPI_WriteEnable(void);
static HAL_StatusTypeDef OSPI_AutoPollingMemReady(uint32_t Timeout);
static HAL_StatusTypeDef OSPI_QuadMode(uint8_t enable);
static HAL_StatusTypeDef OSPI_ResetMemory(void);
static HAL_StatusTypeDef OSPI_MemoryMappedMode(void);
static HAL_StatusTypeDef OSPI_ExitMemoryMappedMode(void);
static HAL_StatusTypeDef OSPI_Init_Internal(void);

/**
  * @brief  Initialize BSS section to zero
  * @note   STM32CubeProgrammer does NOT initialize BSS!
  */
static void InitBSS(void)
{
  uint32_t *pBss = &_sbss;
  while (pBss < &_ebss)
  {
    *pBss++ = 0;
  }
}

/**
  * @brief  System initialization.
  * @retval  1 : Operation succeeded
  * @retval  0 : Operation failed
  */
KeepInCompilation int Init(void)
{
  /* Initialize BSS section FIRST! */
  InitBSS();

  /* Reset of all peripherals */
  HAL_Init();

  /* System Clock Configuration - 실패 시 return 0 */
  if (SystemClock_Config() != 1)
  {
    return 0;
  }

  /* GPIO Initialization */
  MX_GPIO_Init();

  /* OCTOSPI1 Initialization */
  if (OSPI_Init_Internal() != HAL_OK)
  {
    return 0;
  }

  /* Reset the memory */
  if (OSPI_ResetMemory() != HAL_OK)
  {
    return 0;
  }

  /* Enable Quad mode */
  if (OSPI_QuadMode(1) != HAL_OK)
  {
    return 0;
  }

  /* Configure memory mapped mode */
  if (OSPI_MemoryMappedMode() != HAL_OK)
  {
    return 0;
  }

  return 1;
}

/**
  * @brief  Program memory.
  */
KeepInCompilation int Write(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
  uint32_t end_addr, current_size, current_addr;
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* Exit Memory-Mapped mode */
  if (OSPI_ExitMemoryMappedMode() != HAL_OK)
  {
    return 0;
  }

  /* Address translation: 0x90000000 based → flash internal address */
  Address = Address & 0x00FFFFFF;

  /* Calculation of the size between the write address and the end of the page */
  current_size = MEMORY_PAGE_SIZE - (Address % MEMORY_PAGE_SIZE);
  if (current_size > Size)
  {
    current_size = Size;
  }

  current_addr = Address;
  end_addr = Address + Size;

  /* Configure the command for page program (1-1-1 mode) */
  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = PAGE_PROG_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
  sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  do
  {
    sCommand.Address = current_addr;
    sCommand.NbData  = current_size;

    if (OSPI_WriteEnable() != HAL_OK)
    {
      return 0;
    }

    if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return 0;
    }

    if (HAL_OSPI_Transmit(&hospi1, buffer, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return 0;
    }

    if (OSPI_AutoPollingMemReady(HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return 0;
    }

    current_addr += current_size;
    buffer += current_size;
    current_size = ((current_addr + MEMORY_PAGE_SIZE) > end_addr) ? 
                   (end_addr - current_addr) : MEMORY_PAGE_SIZE;
                   
  } while (current_addr < end_addr);

  if (OSPI_MemoryMappedMode() != HAL_OK)
  {
    return 0;
  }

  return 1;
}

/**
  * @brief  Sector erase.
  */
KeepInCompilation int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  if (OSPI_ExitMemoryMappedMode() != HAL_OK)
  {
    return 0;
  }

  EraseStartAddress = EraseStartAddress & 0x00FFFFFF;
  EraseEndAddress = EraseEndAddress & 0x00FFFFFF;
  EraseStartAddress = EraseStartAddress - (EraseStartAddress % MEMORY_BLOCK_SIZE);

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

  while (EraseEndAddress >= EraseStartAddress)
  {
    sCommand.Address = EraseStartAddress;

    if (OSPI_WriteEnable() != HAL_OK)
    {
      return 0;
    }

    if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      return 0;
    }

    if (OSPI_AutoPollingMemReady(400000) != HAL_OK)
    {
      return 0;
    }

    EraseStartAddress += MEMORY_BLOCK_SIZE;
  }

  if (OSPI_MemoryMappedMode() != HAL_OK)
  {
    return 0;
  }

  return 1;
}

/**
  * @brief  Mass erase of the entire chip.
  */
KeepInCompilation int MassErase(uint32_t Parallelism)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  if (OSPI_ExitMemoryMappedMode() != HAL_OK)
  {
    return 0;
  }

  if (OSPI_WriteEnable() != HAL_OK)
  {
    return 0;
  }

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

  if (OSPI_AutoPollingMemReady(400000) != HAL_OK)
  {
    return 0;
  }

  if (OSPI_MemoryMappedMode() != HAL_OK)
  {
    return 0;
  }

  return 1;
}

/**
  * @brief  Verify flash memory with checksum.
  */
KeepInCompilation uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr, 
                                   uint32_t Size, uint32_t missalignement)
{
  uint32_t VerifiedData = 0;
  uint64_t checksum = 0;
  Size *= 4;

  /* Calculate checksum */
  uint32_t startAddr = MemoryAddr + (missalignement & 0xF);
  uint32_t alignedStart = startAddr - (startAddr % 4);
  uint32_t i;
  
  for (i = 0; i < Size; i += 4)
  {
    checksum += *(uint32_t*)(alignedStart + i);
  }

  /* Verify byte by byte */
  while (Size > VerifiedData)
  {
    if (*(uint8_t*)(MemoryAddr + VerifiedData + missalignement) != 
        *((uint8_t*)RAMBufferAddr + VerifiedData))
    {
      return ((checksum << 32) + (MemoryAddr + VerifiedData));
    }
    VerifiedData++;
  }

  return (checksum << 32);
}

/**
  * @brief  Read data from memory.
  */
KeepInCompilation int Read(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

  if (OSPI_ExitMemoryMappedMode() != HAL_OK)
  {
    return 0;
  }

  Address = Address & 0x00FFFFFF;

  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction        = READ_CMD;
  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Address            = Address;
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

  if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return 0;
  }

  if (HAL_OSPI_Receive(&hospi1, buffer, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return 0;
  }

  if (OSPI_MemoryMappedMode() != HAL_OK)
  {
    return 0;
  }

  return 1;
}

/* ============================================================================
 * Private functions
 * ============================================================================ */

/**
  * @brief  System Clock Configuration
  * @note   단순한 클럭 설정 - PLL 없이 MSI 사용!
  *         PLL 설정 실패 가능성을 제거하여 안정성 향상
  * @retval 1: Success, 0: Fail
  */
static int SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* Configure voltage scaling - 반환값 체크 */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    return 0;
  }

  /* ========================================
   * MSI만 사용 (PLL 없이!) - 가장 안정적
   * MSI 48MHz 직접 사용
   * ======================================== */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;  /* 48 MHz */
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;      /* PLL 사용 안 함! */

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    return 0;
  }

  /* Configure system clocks - MSI를 SYSCLK로 직접 사용 */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                              | RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;  /* MSI 직접 사용 */
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    return 0;
  }

  return 1;
}

/**
  * @brief GPIO Initialization
  */
static void MX_GPIO_Init(void)
{
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
  * @brief  OSPI Initialization
  * @note   OSPI 클럭을 MSI로 설정 (PLL 의존성 제거)
  */
static HAL_StatusTypeDef OSPI_Init_Internal(void)
{
  OSPIM_CfgTypeDef sOspiManagerCfg = {0};
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /* ========================================
   * OSPI 클럭을 SYSCLK로 설정 (MSI 48MHz)
   * PLL1 대신 SYSCLK 사용으로 안정성 향상
   * ======================================== */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
  PeriphClkInit.OspiClockSelection = RCC_OSPICLKSOURCE_SYSCLK;  /* SYSCLK (MSI 48MHz) */
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Enable clocks */
  __HAL_RCC_OSPIM_CLK_ENABLE();
  __HAL_RCC_OSPI1_CLK_ENABLE();

  /* Configure GPIO pins: PA2=NCS, PA3=CLK, PA6=IO3, PA7=IO2, PB0=IO1, PB1=IO0 */
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Initialize OSPI handle */
  hospi1.Instance = OCTOSPI1;
  hospi1.Init.FifoThreshold = 4;
  hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
  hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
  hospi1.Init.DeviceSize = 24;  /* 2^24 = 16MB */
  hospi1.Init.ChipSelectHighTime = 2;
  hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
  hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
  hospi1.Init.ClockPrescaler = 4;  /* 48MHz / 4 = 12MHz OSPI clock (안전한 속도) */
  hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
  hospi1.Init.ChipSelectBoundary = 0;
  hospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
  hospi1.Init.MaxTran = 0;
  hospi1.Init.Refresh = 0;

  if (HAL_OSPI_Init(&hospi1) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Configure OSPI Manager */
  sOspiManagerCfg.ClkPort = 1;
  sOspiManagerCfg.NCSPort = 1;
  sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;

  if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  Write Enable
  */
static HAL_StatusTypeDef OSPI_WriteEnable(void)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  OSPI_AutoPollingTypeDef sConfig = {0};

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

  if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  sCommand.Instruction = READ_STATUS_REG_CMD;
  sCommand.DataMode    = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData      = 1;

  sConfig.Match        = STATUS_REG_WEL_MASK;
  sConfig.Mask         = STATUS_REG_WEL_MASK;
  sConfig.MatchMode    = HAL_OSPI_MATCH_MODE_AND;
  sConfig.Interval     = 0x10;
  sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_OSPI_AutoPolling(&hospi1, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  Wait for memory ready
  */
static HAL_StatusTypeDef OSPI_AutoPollingMemReady(uint32_t Timeout)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  OSPI_AutoPollingTypeDef sConfig = {0};

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

  sConfig.Match         = 0;
  sConfig.Mask          = STATUS_REG_WIP_MASK;
  sConfig.MatchMode     = HAL_OSPI_MATCH_MODE_AND;
  sConfig.Interval      = 0x10;
  sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_OSPI_AutoPolling(&hospi1, &sConfig, Timeout) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  Enable/Disable Quad mode
  */
static HAL_StatusTypeDef OSPI_QuadMode(uint8_t enable)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  uint8_t reg[2];

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

  if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Receive(&hospi1, &reg[0], HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  sCommand.Instruction = READ_CFG_REG_CMD;

  if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Receive(&hospi1, &reg[1], HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (enable)
  {
    reg[1] |= CFG_REG_QUAD_MASK;
  }
  else
  {
    reg[1] &= ~CFG_REG_QUAD_MASK;
  }

  if (OSPI_WriteEnable() != HAL_OK)
  {
    return HAL_ERROR;
  }

  sCommand.Instruction = WRITE_STATUS_CFG_REG_CMD;
  sCommand.NbData      = 2;

  if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Transmit(&hospi1, reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (OSPI_AutoPollingMemReady(HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  Reset the memory
  */
static HAL_StatusTypeDef OSPI_ResetMemory(void)
{
  OSPI_RegularCmdTypeDef sCommand = {0};

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

  if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  sCommand.Instruction = RESET_MEMORY_CMD;

  if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  HAL_Delay(1);

  return HAL_OK;
}

/**
  * @brief  Configure memory mapped mode (Quad I/O Read)
  */
static HAL_StatusTypeDef OSPI_MemoryMappedMode(void)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  OSPI_MemoryMappedTypeDef sMemMappedCfg = {0};

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

  if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;

  if (HAL_OSPI_MemoryMapped(&hospi1, &sMemMappedCfg) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  Exit memory mapped mode
  */
static HAL_StatusTypeDef OSPI_ExitMemoryMappedMode(void)
{
  if (HAL_OSPI_Abort(&hospi1) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_OSPI_DeInit(&hospi1) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (OSPI_Init_Internal() != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}
