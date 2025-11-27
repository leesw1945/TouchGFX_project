/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @brief   External Loader Implementation for STM32CubeProgrammer
  *          Target: MX25L12833F 16MB Quad SPI Flash
  *          MCU: STM32U5G9ZJT6Q
  ******************************************************************************
  */

#include "stm32u5xx_hal.h"
#include "mx25l12833f.h"

/* Return codes */
#define LOADER_OK   1
#define LOADER_FAIL 0

/* Flash parameters */
#define EXT_FLASH_ADDR      0x90000000
#define EXT_FLASH_SIZE      0x01000000
#define EXT_FLASH_PAGE_SIZE 0x100
#define EXT_FLASH_SECTOR    0x1000

/* OSPI Handle */
OSPI_HandleTypeDef hospi1;

/* Private function prototypes */
static void SystemClock_Config(void);
static void SystemPower_Config(void);
static void MX_OCTOSPI1_Init(void);

/* ============================================================================
   EXPORTED FUNCTIONS - These are called by STM32CubeProgrammer
   ============================================================================ */

/**
  * @brief  Initialize the external flash
  * @retval LOADER_OK (1) on success, LOADER_FAIL (0) on failure
  */
__attribute__((used, section(".text.Init")))
int Init(void)
{
    uint8_t id[3];
    
    /* MCU initialization */
    HAL_Init();
    SystemPower_Config();
    SystemClock_Config();
    
    /* OCTOSPI initialization */
    MX_OCTOSPI1_Init();
    
    /* Flash initialization */
    if (MX25L12833F_Init(&hospi1) != MX25L12833F_OK)
    {
        return LOADER_FAIL;
    }
    
    /* Verify Flash ID */
    if (MX25L12833F_ReadID(&hospi1, id) != MX25L12833F_OK)
    {
        return LOADER_FAIL;
    }
    
    /* Check Manufacturer ID (0xC2 = Macronix) */
    if (id[0] != 0xC2)
    {
        return LOADER_FAIL;
    }
    
    return LOADER_OK;
}

/**
  * @brief  De-initialize
  * @retval LOADER_OK (1)
  */
__attribute__((used, section(".text.DeInit")))
int DeInit(void)
{
    HAL_OSPI_Abort(&hospi1);
    HAL_OSPI_DeInit(&hospi1);
    return LOADER_OK;
}

/**
  * @brief  Read from external flash
  * @param  Address: Memory address
  * @param  Size: Number of bytes
  * @param  buffer: Destination buffer
  * @retval LOADER_OK or LOADER_FAIL
  */
__attribute__((used, section(".text.Read")))
int Read(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    uint32_t flashAddr;
    
    HAL_OSPI_Abort(&hospi1);
    
    flashAddr = (Address >= EXT_FLASH_ADDR) ? (Address - EXT_FLASH_ADDR) : Address;
    
    if ((flashAddr + Size) > EXT_FLASH_SIZE)
        return LOADER_FAIL;
    
    if (MX25L12833F_Read(&hospi1, buffer, flashAddr, Size) != MX25L12833F_OK)
        return LOADER_FAIL;
    
    return LOADER_OK;
}

/**
  * @brief  Write to external flash
  * @param  Address: Memory address
  * @param  Size: Number of bytes
  * @param  buffer: Source buffer
  * @retval LOADER_OK or LOADER_FAIL
  */
__attribute__((used, section(".text.Write")))
int Write(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    uint32_t flashAddr, currentAddr, endAddr, currentSize;
    uint8_t *pData = buffer;
    
    HAL_OSPI_Abort(&hospi1);
    
    flashAddr = (Address >= EXT_FLASH_ADDR) ? (Address - EXT_FLASH_ADDR) : Address;
    
    if ((flashAddr + Size) > EXT_FLASH_SIZE)
        return LOADER_FAIL;
    
    currentAddr = flashAddr;
    endAddr = flashAddr + Size;
    
    while (currentAddr < endAddr)
    {
        currentSize = EXT_FLASH_PAGE_SIZE - (currentAddr % EXT_FLASH_PAGE_SIZE);
        if (currentSize > (endAddr - currentAddr))
            currentSize = endAddr - currentAddr;
        
        if (MX25L12833F_PageProgram(&hospi1, pData, currentAddr, currentSize) != MX25L12833F_OK)
            return LOADER_FAIL;
        
        currentAddr += currentSize;
        pData += currentSize;
    }
    
    return LOADER_OK;
}

/**
  * @brief  Erase sectors
  * @param  StartAddress: Start address
  * @param  EndAddress: End address
  * @retval LOADER_OK or LOADER_FAIL
  */
__attribute__((used, section(".text.SectorErase")))
int SectorErase(uint32_t StartAddress, uint32_t EndAddress)
{
    uint32_t startAddr, endAddr, sectorAddr;
    
    HAL_OSPI_Abort(&hospi1);
    
    startAddr = (StartAddress >= EXT_FLASH_ADDR) ? (StartAddress - EXT_FLASH_ADDR) : StartAddress;
    endAddr = (EndAddress >= EXT_FLASH_ADDR) ? (EndAddress - EXT_FLASH_ADDR) : EndAddress;
    
    if (endAddr > EXT_FLASH_SIZE)
        endAddr = EXT_FLASH_SIZE;
    
    startAddr = (startAddr / EXT_FLASH_SECTOR) * EXT_FLASH_SECTOR;
    
    for (sectorAddr = startAddr; sectorAddr < endAddr; sectorAddr += EXT_FLASH_SECTOR)
    {
        if (MX25L12833F_Erase_Sector(&hospi1, sectorAddr) != MX25L12833F_OK)
            return LOADER_FAIL;
    }
    
    return LOADER_OK;
}

/**
  * @brief  Mass erase (entire chip)
  * @retval LOADER_OK or LOADER_FAIL
  */
__attribute__((used, section(".text.MassErase")))
int MassErase(void)
{
    HAL_OSPI_Abort(&hospi1);
    
    if (MX25L12833F_Erase_Chip(&hospi1) != MX25L12833F_OK)
        return LOADER_FAIL;
    
    return LOADER_OK;
}

/**
  * @brief  Checksum calculation
  * @param  StartAddress: Start address
  * @param  Size: Size
  * @param  InitVal: Initial value
  * @retval Checksum
  */
__attribute__((used, section(".text.CheckSum")))
uint32_t CheckSum(uint32_t StartAddress, uint32_t Size, uint32_t InitVal)
{
    uint8_t data;
    uint32_t flashAddr, i, checksum = InitVal;
    
    HAL_OSPI_Abort(&hospi1);
    
    flashAddr = (StartAddress >= EXT_FLASH_ADDR) ? (StartAddress - EXT_FLASH_ADDR) : StartAddress;
    
    for (i = 0; i < Size; i++)
    {
        if (MX25L12833F_Read(&hospi1, &data, flashAddr + i, 1) == MX25L12833F_OK)
            checksum += data;
    }
    
    return checksum;
}

/**
  * @brief  Verify programmed data
  * @param  MemoryAddr: Memory address
  * @param  RAMBufferAddr: RAM buffer address
  * @param  Size: Size
  * @param  missalignement: Offset
  * @retval LOADER_OK (1) or error address
  */
__attribute__((used, section(".text.Verify")))
uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr, uint32_t Size, uint32_t missalignement)
{
    uint32_t flashAddr, i;
    uint8_t readData;
    uint8_t *pBuffer = (uint8_t*)RAMBufferAddr;
    
    HAL_OSPI_Abort(&hospi1);
    
    flashAddr = (MemoryAddr >= EXT_FLASH_ADDR) ? (MemoryAddr - EXT_FLASH_ADDR) : MemoryAddr;
    
    flashAddr += missalignement;
    pBuffer += missalignement;
    Size -= missalignement;
    
    for (i = 0; i < Size; i++)
    {
        if (MX25L12833F_Read(&hospi1, &readData, flashAddr + i, 1) != MX25L12833F_OK)
            return (MemoryAddr + missalignement + i);
        
        if (readData != pBuffer[i])
            return (MemoryAddr + missalignement + i);
    }
    
    return LOADER_OK;
}

/* ============================================================================
   PRIVATE FUNCTIONS
   ============================================================================ */

static void SystemPower_Config(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_DisableUCPDDeadBattery();
    
    if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
    {
        while(1);
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        while(1);
    }
    
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 10;
    RCC_OscInitStruct.PLL.PLLP = 10;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    RCC_OscInitStruct.PLL.PLLR = 1;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        while(1);
    }
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                | RCC_CLOCKTYPE_PCLK3;
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

static void MX_OCTOSPI1_Init(void)
{
    OSPIM_CfgTypeDef sOspiManagerCfg = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    
    /* OCTOSPI Clock */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
    PeriphClkInit.OspiClockSelection = RCC_OSPICLKSOURCE_PLL1;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
    
    /* Enable clocks */
    __HAL_RCC_OSPIM_CLK_ENABLE();
    __HAL_RCC_OSPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* GPIO: PA2=NCS, PA3=CLK, PA6=IO3, PA7=IO2 */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* GPIO: PB0=IO1, PB1=IO0 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* OCTOSPI1 configuration */
    hospi1.Instance = OCTOSPI1;
    hospi1.Init.FifoThreshold = 4;
    hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
    hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
    hospi1.Init.DeviceSize = 24;
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
    
    HAL_OSPI_Init(&hospi1);
    
    /* OCTOSPI I/O Manager */
    sOspiManagerCfg.ClkPort = 1;
    sOspiManagerCfg.NCSPort = 1;
    sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
    
    HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
}

/* SysTick Handler - Required for HAL_Delay */
void SysTick_Handler(void)
{
    HAL_IncTick();
}
