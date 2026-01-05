/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @author  Fixed Version
  * @brief   External Loader for MX25L12833F Quad-SPI memory
  * 
  * @version v9.0 - Complete Rewrite for PLL1Q 40MHz
  * 
  * [v9 수정사항]
  * 1. PLL1Q 40MHz 클럭 소스 사용 (HSE 16MHz 기반)
  * 2. HAL_OSPI_MspInit 충돌 해결 (로컬에서 모두 처리)
  * 3. 4READ Dummy Cycles 수정 (Mode bits 포함)
  * 4. Static 함수명 변경으로 main.c와 충돌 방지
  * 5. External Loader 전용 최적화
  ******************************************************************************
  */

#include "stm32u5xx_hal.h"
#include <string.h>

/* ============================================================================
 * MX25L12833F Commands (데이터시트 Table 5 기준)
 * ============================================================================ */
#define RESET_ENABLE_CMD                     0x66
#define RESET_MEMORY_CMD                     0x99

#define READ_STATUS_REG_CMD                  0x05
#define READ_CFG_REG_CMD                     0x15
#define WRITE_STATUS_CFG_REG_CMD             0x01

#define WRITE_ENABLE_CMD                     0x06

#define QUAD_INOUT_READ_CMD                  0xEB    /* 4READ */
#define QUAD_PAGE_PROG_CMD                   0x38    /* 4PP */

#define BLOCK_ERASE_64K_CMD                  0xD8    /* BE */
#define CHIP_ERASE_CMD                       0xC7    /* CE */

/* Status/Configuration Register Masks */
#define STATUS_REG_WIP_MASK                  0x01    /* Write In Progress */
#define STATUS_REG_WEL_MASK                  0x02    /* Write Enable Latch */
#define STATUS_REG_QE_MASK                   0x40    /* Quad Enable (bit 6) */

/* Memory Parameters */
#define MEMORY_FLASH_SIZE                    0x01000000  /* 16 MB */
#define MEMORY_BLOCK_SIZE                    0x10000     /* 64KB */
#define MEMORY_PAGE_SIZE                     0x100       /* 256 bytes */

/*
 * Dummy Cycles for 4READ (데이터시트 Table 10, Figure 33)
 * DC[1:0]=00 기본값: Quad IO Fast Read = 6 dummy cycles
 * 이는 Mode bits 2 cycles + Actual dummy 4 cycles를 포함
 * HAL_OSPI_MEMTYPE_MACRONIX 사용 시 자동 처리됨
 */
#define DUMMY_CYCLES_READ_QUAD               6

/* ============================================================================
 * OSPI Handle - 로컬 정의 (extern 의존성 제거)
 * ============================================================================ */
static OSPI_HandleTypeDef hospi1_local;

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================ */
static void Loader_SystemClock_Config(void);
static void Loader_GPIO_Init(void);
static void Loader_OCTOSPI1_Init(void);
static HAL_StatusTypeDef OSPI_WriteEnable(void);
static HAL_StatusTypeDef OSPI_AutoPollingMemReady(uint32_t Timeout);
static HAL_StatusTypeDef OSPI_QuadMode(uint8_t enable);
static HAL_StatusTypeDef OSPI_ResetMemory(void);
static HAL_StatusTypeDef OSPI_EnterMemoryMappedMode(void);
static HAL_StatusTypeDef OSPI_ExitMemoryMappedMode(void);

/* ============================================================================
 * HAL Tick Override (External Loader에서 SysTick 사용 안 함)
 * uwTick은 stm32u5xx_hal.c에서 extern 선언되어 있으므로 여기서 정의
 * ============================================================================ */
volatile uint32_t uwTick_local = 0;

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    (void)TickPriority;
    return HAL_OK;
}

uint32_t HAL_GetTick(void)
{
    /* 간단한 증가 방식 - 정확한 타이밍이 아닌 타임아웃 용도 */
    return uwTick_local++;
}

void HAL_Delay(uint32_t Delay)
{
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = Delay;

    /* 약간의 여유를 위해 루프 */
    while ((HAL_GetTick() - tickstart) < wait)
    {
        __NOP();
    }
}

/* HAL이 사용하는 uwTick 변수를 우리 로컬 변수로 대체 */
void HAL_IncTick(void)
{
    uwTick_local++;
}

/* ============================================================================
 * Init - System Initialization
 * STM32CubeProgrammer가 가장 먼저 호출하는 함수
 * ============================================================================ */
__attribute__((used)) int Init(void)
{
    /* Step 1: 구조체 초기화 (BSS가 초기화되지 않을 수 있음) */
    memset(&hospi1_local, 0, sizeof(hospi1_local));
    uwTick_local = 0;

    /* Step 2: HAL 초기화 */
    HAL_Init();
    
    /* Step 3: 클럭 설정 (HSE 16MHz + PLL1Q 40MHz) */
    Loader_SystemClock_Config();
    
    /* Step 4: GPIO 초기화 */
    Loader_GPIO_Init();

    /* Step 5: OSPI 초기화 */
    Loader_OCTOSPI1_Init();
    
    /* Step 6: Flash Reset */
    if (OSPI_ResetMemory() != HAL_OK)
    {
        return 0;
    }
    
    /* Step 7: Quad Mode 활성화 */
    if (OSPI_QuadMode(1) != HAL_OK)
    {
        return 0;
    }
    
    /* Step 8: Memory-Mapped Mode 진입 */
    if (OSPI_EnterMemoryMappedMode() != HAL_OK)
    {
        return 0;
    }
    
    return 1;
}

/* ============================================================================
 * Write - Quad Page Program (4PP: 0x38)
 *
 * 데이터시트 Figure 49: 4PP Sequence
 * - Command: 1-line (0x38)
 * - Address: 4-lines (6 cycles for 24-bit address)
 * - Data: 4-lines
 * - Dummy: 없음
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
    
    /* 주소 변환: 0x90000000 → 0x00000000 */
    if (Address >= 0x90000000)
    {
        Address -= 0x90000000;
    }
    
    current_addr = Address;
    end_addr = Address + Size;
    
    while (current_addr < end_addr)
    {
        /* Page 내 남은 공간 계산 */
        current_size = MEMORY_PAGE_SIZE - (current_addr % MEMORY_PAGE_SIZE);
        if (current_size > (end_addr - current_addr))
        {
            current_size = end_addr - current_addr;
        }
        
        /* Write Enable */
        if (OSPI_WriteEnable() != HAL_OK)
        {
            return 0;
        }
        
        /* 4PP Command 설정 (데이터시트 Figure 49) */
        sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
        sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
        sCommand.Instruction        = QUAD_PAGE_PROG_CMD;           /* 0x38 */
        sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;  /* Command: 1-line */
        sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
        sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
        sCommand.Address            = current_addr;
        sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;     /* Address: 4-lines */
        sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
        sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
        sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
        sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;        /* Data: 4-lines */
        sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
        sCommand.NbData             = current_size;
        sCommand.DummyCycles        = 0;                            /* 4PP는 dummy 없음 */
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
        
        /* Write 완료 대기 */
        if (OSPI_AutoPollingMemReady(HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return 0;
        }
        
        current_addr += current_size;
        buffer += current_size;
    }
    
    /* Memory-Mapped 모드 복귀 */
    if (OSPI_EnterMemoryMappedMode() != HAL_OK)
    {
        return 0;
    }
    
    return 1;
}

/* ============================================================================
 * SectorErase - 64KB Block Erase (BE: 0xD8)
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
    
    /* 주소 변환 */
    if (EraseStartAddress >= 0x90000000)
    {
        EraseStartAddress -= 0x90000000;
    }
    if (EraseEndAddress >= 0x90000000)
    {
        EraseEndAddress -= 0x90000000;
    }
    
    /* 64KB 경계로 정렬 */
    EraseStartAddress = EraseStartAddress - (EraseStartAddress % MEMORY_BLOCK_SIZE);
    
    while (EraseStartAddress < EraseEndAddress)
    {
        BlockAddr = EraseStartAddress;
        
        /* Write Enable */
        if (OSPI_WriteEnable() != HAL_OK)
        {
            return 0;
        }
        
        /* Block Erase Command (1-1-0) - Standard SPI mode */
        sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
        sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
        sCommand.Instruction        = BLOCK_ERASE_64K_CMD;          /* 0xD8 */
        sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
        sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
        sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
        sCommand.Address            = BlockAddr;
        sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;      /* Standard SPI */
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
        
        /* Erase 완료 대기 (최대 650ms - 데이터시트 참조) */
        if (OSPI_AutoPollingMemReady(1000) != HAL_OK)
        {
            return 0;
        }
        
        EraseStartAddress += MEMORY_BLOCK_SIZE;
    }
    
    /* Memory-Mapped 모드 복귀 */
    if (OSPI_EnterMemoryMappedMode() != HAL_OK)
    {
        return 0;
    }
    
    return 1;
}

/* ============================================================================
 * MassErase - Chip Erase (CE: 0xC7)
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
    
    /* Write Enable */
    if (OSPI_WriteEnable() != HAL_OK)
    {
        return 0;
    }
    
    /* Chip Erase Command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = CHIP_ERASE_CMD;                   /* 0xC7 */
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
    
    /* Chip Erase 완료 대기 (최대 60초 - 데이터시트 참조) */
    if (OSPI_AutoPollingMemReady(60000) != HAL_OK)
    {
        return 0;
    }
    
    /* Memory-Mapped 모드 복귀 */
    if (OSPI_EnterMemoryMappedMode() != HAL_OK)
    {
        return 0;
    }
    
    return 1;
}

/* ============================================================================
 * Read - Memory-Mapped 모드에서 직접 읽기
 * ============================================================================ */
__attribute__((used)) int Read(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    memcpy(buffer, (uint8_t*)Address, Size);
    return 1;
}

/* ============================================================================
 * Verify - 검증
 * ============================================================================ */
__attribute__((used)) uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr, 
                                       uint32_t Size, uint32_t missalignement)
{
    uint32_t VerifiedData = 0;
    uint64_t checksum = 0;
    
    (void)missalignement;

    Size *= 4;
    
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
 * Loader_SystemClock_Config - HSE 16MHz + PLL1Q 40MHz
 *
 * ★★★ 핵심: OSPI Clock = PLL1Q = 40MHz ★★★
 *
 * HSE = 16MHz
 * PLL1: M=1, N=10, P=2, Q=4, R=1
 * - PLLCLK (R) = 16 * 10 / 1 = 160MHz (SYSCLK용)
 * - PLL1Q = 16 * 10 / 4 = 40MHz (OSPI용)
 * ============================================================================ */
static void Loader_SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    
    /* PWR 클럭 활성화 */
    __HAL_RCC_PWR_CLK_ENABLE();
    
    /* Voltage Scaling - 고속 동작을 위해 Scale 1 사용 */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        return;
    }
    
    /* HSE 16MHz + PLL 설정 */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
    RCC_OscInitStruct.PLL.PLLM = 1;     /* VCO input = 16MHz / 1 = 16MHz */
    RCC_OscInitStruct.PLL.PLLN = 10;    /* VCO output = 16 * 10 = 160MHz */
    RCC_OscInitStruct.PLL.PLLP = 2;     /* PLLP = 160 / 2 = 80MHz */
    RCC_OscInitStruct.PLL.PLLQ = 4;     /* PLLQ = 160 / 4 = 40MHz ← OSPI 클럭 */
    RCC_OscInitStruct.PLL.PLLR = 1;     /* PLLR = 160 / 1 = 160MHz (SYSCLK) */
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        return;
    }
    
    /* 시스템 클럭 설정 - PLL을 SYSCLK 소스로 */
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

    /* ★★★ OSPI Clock Source: PLL1Q (40MHz) ★★★ */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
    PeriphClkInit.OspiClockSelection = RCC_OSPICLKSOURCE_PLL1;  /* PLL1Q 사용! */

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        return;
    }
}

/* ============================================================================
 * Loader_GPIO_Init - GPIO 초기화
 * ============================================================================ */
static void Loader_GPIO_Init(void)
{
    /* GPIO 클럭 활성화 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

/* ============================================================================
 * Loader_OCTOSPI1_Init - OSPI 초기화 (로컬 버전)
 *
 * OSPI Clock = PLL1Q = 40MHz
 * Flash Clock = 40MHz / 1 = 40MHz (Prescaler=1)
 * Flash max = 133MHz (4READ 기준) → 40MHz는 안전한 범위
 * ============================================================================ */
static void Loader_OCTOSPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    OSPIM_CfgTypeDef sOspiManagerCfg = {0};

    /* OSPI 클럭 활성화 */
    __HAL_RCC_OSPIM_CLK_ENABLE();
    __HAL_RCC_OSPI1_CLK_ENABLE();

    /* GPIO 클럭 활성화 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* GPIO Configuration
     * PA2  -> OCTOSPIM_P1_NCS
     * PA3  -> OCTOSPIM_P1_CLK
     * PA6  -> OCTOSPIM_P1_IO3
     * PA7  -> OCTOSPIM_P1_IO2
     * PB0  -> OCTOSPIM_P1_IO1
     * PB1  -> OCTOSPIM_P1_IO0
     */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* OSPI 핸들 초기화 */
    hospi1_local.Instance = OCTOSPI1;
    hospi1_local.Init.FifoThreshold = 4;
    hospi1_local.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
//    hospi1_local.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;       /* Macronix type */
    hospi1_local.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
    hospi1_local.Init.DeviceSize = 24;                              /* 2^24 = 16MB */
    hospi1_local.Init.ChipSelectHighTime = 5;                       /* tSHSL >= 30ns */
    hospi1_local.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
    hospi1_local.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;            /* Mode 0 */
    hospi1_local.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
    hospi1_local.Init.ClockPrescaler = 4;                           /* 40MHz / 1 = 40MHz */
//    hospi1_local.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
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

    /* OSPI Manager Configuration */
    sOspiManagerCfg.ClkPort = 1;
    sOspiManagerCfg.NCSPort = 1;
    sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;

    if (HAL_OSPIM_Config(&hospi1_local, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return;
    }
}

/* ============================================================================
 * OSPI_WriteEnable - Write Enable (WREN: 0x06)
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_WriteEnable(void)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_AutoPollingTypeDef sConfig = {0};
    
    /* WREN Command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = WRITE_ENABLE_CMD;                 /* 0x06 */
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
    
    /* WEL 비트 확인 */
    sCommand.Instruction = READ_STATUS_REG_CMD;                     /* 0x05 */
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
 * OSPI_AutoPollingMemReady - WIP 비트 폴링
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_AutoPollingMemReady(uint32_t Timeout)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_AutoPollingTypeDef sConfig = {0};
    
    /* RDSR Command */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = READ_STATUS_REG_CMD;              /* 0x05 */
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
    
    /* WIP=0 대기 */
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
 * OSPI_QuadMode - Quad Enable 비트 설정
 *
 * 데이터시트 Table 7 (Status Register):
 *   bit 6 = QE (Quad Enable)
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_QuadMode(uint8_t enable)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint8_t reg[2] = {0, 0};
    
    /* Step 1: Read Status Register (0x05) → reg[0] */
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
    
    if (HAL_OSPI_Receive(&hospi1_local, &reg[0], HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* Step 2: Read Configuration Register (0x15) → reg[1] */
    sCommand.Instruction = READ_CFG_REG_CMD;
    
    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    if (HAL_OSPI_Receive(&hospi1_local, &reg[1], HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* QE 비트가 이미 설정되어 있으면 스킵 */
    if (enable && (reg[0] & STATUS_REG_QE_MASK))
    {
        return HAL_OK;
    }

    /* Step 3: QE 비트 설정 (Status Register bit 6) */
    if (enable)
    {
        reg[0] |= STATUS_REG_QE_MASK;
    }
    else
    {
        reg[0] &= ~STATUS_REG_QE_MASK;
    }
    
    /* Step 4: Write Enable */
    if (OSPI_WriteEnable() != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* Step 5: Write Status & Configuration Register (0x01) */
    sCommand.Instruction = WRITE_STATUS_CFG_REG_CMD;
    sCommand.NbData      = 2;
    
    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    if (HAL_OSPI_Transmit(&hospi1_local, reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* Step 6: Write 완료 대기 */
    if (OSPI_AutoPollingMemReady(HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/* ============================================================================
 * OSPI_ResetMemory - Software Reset
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ResetMemory(void)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    
    /* Reset Enable (0x66) */
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
    
    /* Reset Memory (0x99) */
    sCommand.Instruction = RESET_MEMORY_CMD;
    
    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* Reset 완료 대기 (데이터시트: tRST = 35us max) */
    HAL_Delay(1);
    
    return HAL_OK;
}

/* ============================================================================
 * OSPI_EnterMemoryMappedMode - 4READ (0xEB) Memory-Mapped Mode
 *
 * 데이터시트 Figure 33 - 4READ Sequence (SPI Mode):
 * [CMD:1-line 8clk] [ADDR:4-line 6cyc] [Mode:4-line 2cyc] [Dummy:4cyc] [DATA:4-line]
 *
 * Dummy Cycles = 6 (DC[1:0] = 00 기본값, 데이터시트 Table 10)
 * HAL_OSPI_MEMTYPE_MACRONIX 설정 시 Mode bits는 HAL이 자동 처리
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_EnterMemoryMappedMode(void)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_MemoryMappedTypeDef sMemMappedCfg = {0};
    
    /* ========== Read Configuration (4READ: 0xEB) ========== */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_READ_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = QUAD_INOUT_READ_CMD;              /* 0xEB */
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = 0;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;

//    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;    /* MACRONIX type이 처리 */
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
    sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
    sCommand.AlternateBytes     = 0x00;

    sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData             = 0;

//    sCommand.DummyCycles        = DUMMY_CYCLES_READ_QUAD;
    sCommand.DummyCycles        = 4;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* ========== Write Configuration (4PP: 0x38) ========== */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_WRITE_CFG;
    sCommand.Instruction        = QUAD_PAGE_PROG_CMD;               /* 0x38 */
    sCommand.DummyCycles        = 0;                                /* 4PP는 dummy 없음 */

    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* ========== Memory-Mapped Mode 활성화 ========== */
    sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;
    
    if (HAL_OSPI_MemoryMapped(&hospi1_local, &sMemMappedCfg) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/* ============================================================================
 * OSPI_ExitMemoryMappedMode - Memory-Mapped Mode 종료
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ExitMemoryMappedMode(void)
{
    /* Abort */
    if (HAL_OSPI_Abort(&hospi1_local) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* DeInit 및 재초기화 */
    HAL_OSPI_DeInit(&hospi1_local);

    /* 구조체 초기화 */
    memset(&hospi1_local, 0, sizeof(hospi1_local));
    
    /* 재초기화 */
    Loader_OCTOSPI1_Init();
    
    return HAL_OK;
}
