/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @author  Diagnostic Version v13 - Memory-Mapped Mode for Read
  * @brief   SPI 1-Line 모드 + Memory-Mapped 모드
  * 
  * ★★★ STM32CubeProgrammer는 Read 시 직접 0x90000000 접근 ★★★
  * 따라서 Init 후 Memory-Mapped 모드로 있어야 함
  ******************************************************************************
  */

#include "stm32u5xx_hal.h"
#include <string.h>

/* ============================================================================
 * MX25L12833F Commands
 * ============================================================================ */
#define RESET_ENABLE_CMD                     0x66
#define RESET_MEMORY_CMD                     0x99
#define READ_ID_CMD                          0x9F
#define READ_CMD                             0x03    /* Normal Read (1-1-1) */
#define FAST_READ_CMD                        0x0B    /* Fast Read (1-1-1) */
#define PAGE_PROG_CMD                        0x02    /* Page Program (1-1-1) */
#define READ_STATUS_REG_CMD                  0x05
#define WRITE_ENABLE_CMD                     0x06
#define BLOCK_ERASE_64K_CMD                  0xD8
#define CHIP_ERASE_CMD                       0xC7

/* Status Register Masks */
#define STATUS_REG_WIP_MASK                  0x01
#define STATUS_REG_WEL_MASK                  0x02

/* Memory Parameters */
#define MEMORY_FLASH_SIZE                    0x01000000
#define MEMORY_BLOCK_SIZE                    0x10000
#define MEMORY_PAGE_SIZE                     0x100

/* Expected ID */
#define MX25L12833F_MANUFACTURER_ID          0xC2
#define MX25L12833F_MEMORY_TYPE              0x20
#define MX25L12833F_MEMORY_DENSITY           0x18

/* ============================================================================
 * OSPI Handle & 진단 변수
 * ============================================================================ */
static OSPI_HandleTypeDef hospi1_local;
static uint8_t g_flash_id[3] = {0, 0, 0};
static uint8_t g_init_error_code = 0;
static uint8_t g_status_reg = 0;

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================ */
static void Loader_SystemClock_Config(void);
static void Loader_OCTOSPI1_Init(void);
static HAL_StatusTypeDef OSPI_WriteEnable(void);
static HAL_StatusTypeDef OSPI_AutoPollingMemReady(uint32_t Timeout);
static HAL_StatusTypeDef OSPI_ResetMemory(void);
static HAL_StatusTypeDef OSPI_ReadID(uint8_t* id);
static HAL_StatusTypeDef OSPI_ReadStatusReg(uint8_t* status);
static HAL_StatusTypeDef OSPI_EnterMemoryMappedMode(void);
static HAL_StatusTypeDef OSPI_ExitMemoryMappedMode(void);

/* ============================================================================
 * HAL Tick Override
 * ============================================================================ */
volatile uint32_t uwTick_local = 0;

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    (void)TickPriority;
    return HAL_OK;
}

uint32_t HAL_GetTick(void)
{
    return uwTick_local++;
}

void HAL_Delay(uint32_t Delay)
{
    uint32_t tickstart = HAL_GetTick();
    while ((HAL_GetTick() - tickstart) < Delay)
    {
        __NOP();
    }
}

void HAL_IncTick(void)
{
    uwTick_local++;
}

/* ============================================================================
 * OSPI_ReadID - Flash ID 읽기 (0x9F)
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ReadID(uint8_t* id)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

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

    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_OSPI_Receive(&hospi1_local, id, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ============================================================================
 * OSPI_ReadStatusReg
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ReadStatusReg(uint8_t* status)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

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

    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_OSPI_Receive(&hospi1_local, status, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ============================================================================
 * OSPI_EnterMemoryMappedMode - SPI 1-Line 모드로 Memory-Mapped
 * Fast Read (0x0B) 사용 - 8 dummy cycles 필요
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_EnterMemoryMappedMode(void)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_MemoryMappedTypeDef sMemMappedCfg = {0};

    /* Read 설정 - Fast Read (0x0B) 1-1-1 모드 */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_READ_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = FAST_READ_CMD;                /* 0x0B */
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = 0;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData             = 0;
    sCommand.DummyCycles        = 8;                            /* Fast Read는 8 dummy cycles */
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Write 설정 - Page Program (0x02) 1-1-1 모드 */
    sCommand.OperationType = HAL_OSPI_OPTYPE_WRITE_CFG;
    sCommand.Instruction   = PAGE_PROG_CMD;                     /* 0x02 */
    sCommand.DummyCycles   = 0;

    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Memory-Mapped 모드 활성화 */
    sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;

    if (HAL_OSPI_MemoryMapped(&hospi1_local, &sMemMappedCfg) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ============================================================================
 * OSPI_ExitMemoryMappedMode
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ExitMemoryMappedMode(void)
{
    if (HAL_OSPI_Abort(&hospi1_local) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* OSPI 재초기화 */
    HAL_OSPI_DeInit(&hospi1_local);
    memset(&hospi1_local, 0, sizeof(hospi1_local));
    Loader_OCTOSPI1_Init();

    return HAL_OK;
}

/* ============================================================================
 * Init - System Initialization
 * ============================================================================ */
__attribute__((used)) int Init(void)
{
    g_init_error_code = 0;
    g_flash_id[0] = 0;
    g_flash_id[1] = 0;
    g_flash_id[2] = 0;
    g_status_reg = 0;

    memset(&hospi1_local, 0, sizeof(hospi1_local));
    uwTick_local = 0;

    HAL_Init();
    Loader_SystemClock_Config();
    Loader_OCTOSPI1_Init();
    
    HAL_Delay(100);

    /* Flash Reset */
    if (OSPI_ResetMemory() != HAL_OK)
    {
        g_init_error_code = 1;
        return 0;
    }
    
    HAL_Delay(100);

    /* Flash ID 읽기 */
    if (OSPI_ReadID(g_flash_id) != HAL_OK)
    {
        g_init_error_code = 2;
        return 0;
    }

    /* Status Register 읽기 */
    if (OSPI_ReadStatusReg(&g_status_reg) != HAL_OK)
    {
        g_init_error_code = 3;
        return 0;
    }

    /* ID 검증 */
    if (g_flash_id[0] != MX25L12833F_MANUFACTURER_ID)
    {
        g_init_error_code = 4;
        /* 계속 진행 */
    }

    /* ★★★ Memory-Mapped 모드 진입 ★★★ */
    /* STM32CubeProgrammer가 Read 시 직접 0x90000000 주소 접근하므로 필요 */
    if (OSPI_EnterMemoryMappedMode() != HAL_OK)
    {
        g_init_error_code = 5;
        return 0;
    }

    return 1;
}

/* ============================================================================
 * Read - Memory-Mapped 모드에서 memcpy
 * ============================================================================ */
__attribute__((used)) int Read(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    memcpy(buffer, (uint8_t*)Address, Size);
    return 1;
}

/* ============================================================================
 * Write - Page Program (0x02) - 1-1-1 모드
 * ============================================================================ */
__attribute__((used)) int Write(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint32_t end_addr, current_size, current_addr;
    
    /* Memory-Mapped 모드 종료 */
    if (OSPI_ExitMemoryMappedMode() != HAL_OK)
    {
        return 0;
    }

    if (Address >= 0x90000000)
    {
        Address -= 0x90000000;
    }
    
    current_addr = Address;
    end_addr = Address + Size;
    
    while (current_addr < end_addr)
    {
        current_size = MEMORY_PAGE_SIZE - (current_addr % MEMORY_PAGE_SIZE);
        if (current_size > (end_addr - current_addr))
        {
            current_size = end_addr - current_addr;
        }
        
        if (OSPI_WriteEnable() != HAL_OK)
        {
            return 0;
        }
        
        sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
        sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
        sCommand.Instruction        = PAGE_PROG_CMD;
        sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
        sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
        sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
        sCommand.Address            = current_addr;
        sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
        sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
        sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
        sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
        sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
        sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
        sCommand.NbData             = current_size;
        sCommand.DummyCycles        = 0;
        sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
        sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
        
        if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return 0;
        }
        
        if (HAL_OSPI_Transmit(&hospi1_local, buffer, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return 0;
        }
        
        if (OSPI_AutoPollingMemReady(5000) != HAL_OK)
        {
            return 0;
        }
        
        current_addr += current_size;
        buffer += current_size;
    }
    
    /* Memory-Mapped 모드 재진입 */
    if (OSPI_EnterMemoryMappedMode() != HAL_OK)
    {
        return 0;
    }

    return 1;
}

/* ============================================================================
 * SectorErase - 64KB Block Erase
 * ============================================================================ */
__attribute__((used)) int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint32_t BlockAddr;
    
    /* Memory-Mapped 모드 종료 */
    if (OSPI_ExitMemoryMappedMode() != HAL_OK)
    {
        return 0;
    }

    if (EraseStartAddress >= 0x90000000)
    {
        EraseStartAddress -= 0x90000000;
    }
    if (EraseEndAddress >= 0x90000000)
    {
        EraseEndAddress -= 0x90000000;
    }
    
    EraseStartAddress = EraseStartAddress - (EraseStartAddress % MEMORY_BLOCK_SIZE);
    
    while (EraseStartAddress < EraseEndAddress)
    {
        BlockAddr = EraseStartAddress;
        
        if (OSPI_WriteEnable() != HAL_OK)
        {
            return 0;
        }
        
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
        
        if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return 0;
        }
        
        if (OSPI_AutoPollingMemReady(5000) != HAL_OK)
        {
            return 0;
        }
        
        EraseStartAddress += MEMORY_BLOCK_SIZE;
    }
    
    /* Memory-Mapped 모드 재진입 */
    if (OSPI_EnterMemoryMappedMode() != HAL_OK)
    {
        return 0;
    }

    return 1;
}

/* ============================================================================
 * MassErase
 * ============================================================================ */
__attribute__((used)) int MassErase(uint32_t Parallelism)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    (void)Parallelism;
    
    /* Memory-Mapped 모드 종료 */
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
    
    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }
    
    if (OSPI_AutoPollingMemReady(100000) != HAL_OK)
    {
        return 0;
    }
    
    /* Memory-Mapped 모드 재진입 */
    if (OSPI_EnterMemoryMappedMode() != HAL_OK)
    {
        return 0;
    }

    return 1;
}

/* ============================================================================
 * Verify
 * ============================================================================ */
__attribute__((used)) uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr, 
                                       uint32_t Size, uint32_t missalignement)
{
    uint32_t VerifiedData = 0;
    uint64_t checksum = 0;
    
    (void)missalignement;
    Size *= 4;
    
    /* Memory-Mapped 모드에서 직접 비교 */
    while (Size > VerifiedData)
    {
        if (*(uint8_t*)(MemoryAddr + VerifiedData) != *(uint8_t*)(RAMBufferAddr + VerifiedData))
        {
            return ((checksum << 32) + (MemoryAddr + VerifiedData));
        }
        checksum += *(uint8_t*)(MemoryAddr + VerifiedData);
        VerifiedData++;
    }
    
    return (checksum << 32);
}

/* ============================================================================
 * Loader_SystemClock_Config
 * ============================================================================ */
static void Loader_SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    
    __HAL_RCC_PWR_CLK_ENABLE();
    
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        return;
    }
    
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 10;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    RCC_OscInitStruct.PLL.PLLR = 1;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        return;
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
        return;
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
    PeriphClkInit.OspiClockSelection = RCC_OSPICLKSOURCE_PLL1;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        return;
    }
}

/* ============================================================================
 * Loader_OCTOSPI1_Init - 5MHz, SPI 모드
 * ============================================================================ */
static void Loader_OCTOSPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    OSPIM_CfgTypeDef sOspiManagerCfg = {0};

    __HAL_RCC_OSPIM_CLK_ENABLE();
    __HAL_RCC_OSPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* GPIO - Pull-up */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* OSPI 설정 - 5MHz */
    hospi1_local.Instance = OCTOSPI1;
    hospi1_local.Init.FifoThreshold = 4;
    hospi1_local.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
    hospi1_local.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
    hospi1_local.Init.DeviceSize = 24;
    hospi1_local.Init.ChipSelectHighTime = 8;
    hospi1_local.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
    hospi1_local.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
    hospi1_local.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
    hospi1_local.Init.ClockPrescaler = 8;                           /* 5MHz */
    hospi1_local.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_HALFCYCLE;
    hospi1_local.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
    hospi1_local.Init.ChipSelectBoundary = 0;
    hospi1_local.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
    hospi1_local.Init.MaxTran = 0;
    hospi1_local.Init.Refresh = 0;

    if (HAL_OSPI_Init(&hospi1_local) != HAL_OK)
    {
        return;
    }

    sOspiManagerCfg.ClkPort = 1;
    sOspiManagerCfg.NCSPort = 1;
    sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;

    if (HAL_OSPIM_Config(&hospi1_local, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return;
    }
}

/* ============================================================================
 * OSPI_WriteEnable
 * ============================================================================ */
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
    
    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    sCommand.Instruction = READ_STATUS_REG_CMD;
    sCommand.DataMode    = HAL_OSPI_DATA_1_LINE;
    sCommand.NbData      = 1;
    sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    
    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    sConfig.Match         = STATUS_REG_WEL_MASK;
    sConfig.Mask          = STATUS_REG_WEL_MASK;
    sConfig.MatchMode     = HAL_OSPI_MATCH_MODE_AND;
    sConfig.Interval      = 0x10;
    sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;
    
    if (HAL_OSPI_AutoPolling(&hospi1_local, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/* ============================================================================
 * OSPI_AutoPollingMemReady
 * ============================================================================ */
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
    
    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    sConfig.Match         = 0x00;
    sConfig.Mask          = STATUS_REG_WIP_MASK;
    sConfig.MatchMode     = HAL_OSPI_MATCH_MODE_AND;
    sConfig.Interval      = 0x10;
    sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;
    
    if (HAL_OSPI_AutoPolling(&hospi1_local, &sConfig, Timeout) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/* ============================================================================
 * OSPI_ResetMemory
 * ============================================================================ */
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
    
    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    sCommand.Instruction = RESET_MEMORY_CMD;
    
    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    HAL_Delay(10);
    
    return HAL_OK;
}
