/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @author  MCD Application Team
  * @brief   External Loader for MX25L12833F Quad-SPI memory
  * 
  * @version v7.1 - 빌드 에러 수정
  * 
  * [v7 수정사항 - 데이터시트 PM2517 Rev 1.0 기준]
  * 1. QE 비트 위치 수정: Status Register bit 6 (reg[0], NOT reg[1])
  * 2. 4READ Dummy Cycles 수정: AlternateBytes(2) + Dummy(4) = 6 total
  * 3. 4PP Address Mode 수정: 4-lines (NOT 1-line)
  * 4. Clock 설정 단순화: MSI 48MHz 직접 사용 (PLL 없음)
  * 5. OSPI Clock Source: SYSCLK 직접 사용
  * 6. BSS 수동 초기화 추가
  ******************************************************************************
  */

#include "Loader_Src.h"
#include "octospi.h"
#include "string.h"

/* ============================================================================
 * 헤더 파일의 정의를 덮어쓰기 (v7 수정)
 * ============================================================================ */
#undef DUMMY_CYCLES_READ_QUAD
#define DUMMY_CYCLES_READ_QUAD               4   /* v7: 6-2=4 (Mode Bits 제외) */

/* Status Register QE 비트 (헤더에 없으면 추가) */
#ifndef STATUS_REG_QE_MASK
#define STATUS_REG_QE_MASK                   0x40  /* bit 6: Quad Enable */
#endif

/* ============================================================================
 * External Variables (octospi.c에서 정의됨)
 * ============================================================================ */
extern OSPI_HandleTypeDef hospi1;

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================ */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static HAL_StatusTypeDef OSPI_WriteEnable(void);
static HAL_StatusTypeDef OSPI_AutoPollingMemReady(uint32_t Timeout);
static HAL_StatusTypeDef OSPI_QuadMode(uint8_t enable);
static HAL_StatusTypeDef OSPI_ResetMemory(void);
static HAL_StatusTypeDef OSPI_EnterMemoryMappedMode(void);
static HAL_StatusTypeDef OSPI_ExitMemoryMappedMode(void);

/* ============================================================================
 * BSS Initialization (STM32CubeProgrammer doesn't clear BSS!)
 * ============================================================================ */
static void Init_BSS(void)
{
    /* hospi1 구조체 초기화 - octospi.c의 전역 변수 */
    memset(&hospi1, 0, sizeof(hospi1));
}

/* ============================================================================
 * Init - System Initialization
 * ============================================================================ */
__attribute__((used)) int Init(void)
{
    /* Step 1: BSS 수동 초기화 (필수!) */
    Init_BSS();
    
    /* Step 2: HAL 초기화 */
    HAL_Init();
    
    /* Step 3: 클럭 설정 (MSI 48MHz, PLL 없음) */
    SystemClock_Config();
    
    /* Step 4: GPIO 초기화 */
    MX_GPIO_Init();
    
    /* Step 5: OSPI 초기화 (CubeMX 생성 함수 사용) */
    MX_OCTOSPI1_Init();
    
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
 * Write - Page Program (4PP: 0x38)
 * 
 * Datasheet Figure 49: 4PP Sequence
 * - Command: 1-line (0x38)
 * - Address: 4-lines (NOT 1-line!)
 * - Data: 4-lines
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
        
        /* 4PP Command 설정 (Datasheet Figure 49) */
        sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
        sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
        sCommand.Instruction        = QUAD_PAGE_PROG_CMD;           /* 0x38 */
        sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;  /* Command: 1-line */
        sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
        sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
        sCommand.Address            = current_addr;
        sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;     /* ★ v7: 4-lines! */
        sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
        sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
        sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
        sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;        /* Data: 4-lines */
        sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
        sCommand.NbData             = current_size;
        sCommand.DummyCycles        = 0;
        sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
        sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
        
        if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return 0;
        }
        
        if (HAL_OSPI_Transmit(&hospi1, buffer, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
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
    
    while (EraseStartAddress <= EraseEndAddress)
    {
        BlockAddr = EraseStartAddress;
        
        /* Write Enable */
        if (OSPI_WriteEnable() != HAL_OK)
        {
            return 0;
        }
        
        /* Block Erase Command (1-1-0) */
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
        
        /* Erase 완료 대기 */
        if (OSPI_AutoPollingMemReady(400000) != HAL_OK)
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
    
    /* Chip Erase 완료 대기 */
    if (OSPI_AutoPollingMemReady(400000) != HAL_OK)
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
 * SystemClock_Config - MSI 48MHz 직접 사용 (PLL 없음, 안정적)
 * ============================================================================ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    /* PWR 클럭 활성화 */
    __HAL_RCC_PWR_CLK_ENABLE();
    
    /* Voltage Scaling */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        return;
    }
    
    /* MSI 48MHz 설정 */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;  /* 48 MHz */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        return;
    }
    
    /* 시스템 클럭을 MSI로 직접 설정 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                | RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        return;
    }
}

/* ============================================================================
 * MX_GPIO_Init
 * ============================================================================ */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
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
    
    /* WEL 비트 확인 */
    sCommand.Instruction = READ_STATUS_REG_CMD;
    sCommand.DataMode    = HAL_OSPI_DATA_1_LINE;
    sCommand.NbData      = 1;
    sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    
    if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    sConfig.Match         = STATUS_REG_WEL_MASK;
    sConfig.Mask          = STATUS_REG_WEL_MASK;
    sConfig.MatchMode     = HAL_OSPI_MATCH_MODE_AND;
    sConfig.Interval      = 0x10;
    sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;
    
    if (HAL_OSPI_AutoPolling(&hospi1, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
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
    
    /* WIP=0 대기 */
    sConfig.Match         = 0x00;
    sConfig.Mask          = STATUS_REG_WIP_MASK;
    sConfig.MatchMode     = HAL_OSPI_MATCH_MODE_AND;
    sConfig.Interval      = 0x10;
    sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;
    
    if (HAL_OSPI_AutoPolling(&hospi1, &sConfig, Timeout) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/* ============================================================================
 * OSPI_QuadMode - Quad Enable 비트 설정
 * 
 * ★★★ v7 핵심 수정 ★★★
 * 
 * 데이터시트 Table 7 (Status Register):
 *   bit 6 = QE (Quad Enable)
 * 
 * 데이터시트 Table 8 (Configuration Register):
 *   bit 6 = DC0 (Dummy Cycle 설정) - QE 아님!
 * 
 * 기존 코드 오류: reg[1] |= 0x40 (Configuration Register의 DC0 설정)
 * 수정: reg[0] |= 0x40 (Status Register의 QE 설정)
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
    
    if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    if (HAL_OSPI_Receive(&hospi1, &reg[0], HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* Step 2: Read Configuration Register (0x15) → reg[1] */
    sCommand.Instruction = READ_CFG_REG_CMD;
    
    if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    if (HAL_OSPI_Receive(&hospi1, &reg[1], HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* Step 3: QE 비트 설정 (★ Status Register bit 6!) */
    if (enable)
    {
        reg[0] |= STATUS_REG_QE_MASK;   /* ★ v7: Status Register bit 6 = QE */
    }
    else
    {
        reg[0] &= ~STATUS_REG_QE_MASK;
    }
    
    /* Configuration Register는 그대로 유지 (DC0, DC1 변경 안함) */
    
    /* Step 4: Write Enable */
    if (OSPI_WriteEnable() != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* Step 5: Write Status & Configuration Register (0x01) */
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
    
    if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* Reset Memory (0x99) */
    sCommand.Instruction = RESET_MEMORY_CMD;
    
    if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* Reset 완료 대기 */
    HAL_Delay(1);
    
    return HAL_OK;
}

/* ============================================================================
 * OSPI_EnterMemoryMappedMode - 4READ (0xEB) Memory-Mapped Mode
 * 
 * 데이터시트 Figure 33 - 4READ Sequence:
 * [CMD:1-line] [ADDR:4-line 6cycles] [MODE:4-line 2cycles] [DUMMY:4cycles] [DATA:4-line]
 * 
 * Total Dummy = Mode Bits (2 cycles) + Additional Dummy (4 cycles) = 6 cycles
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_EnterMemoryMappedMode(void)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_MemoryMappedTypeDef sMemMappedCfg = {0};
    
    /* 4READ Command Configuration */
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
    
    /* ★ v7: Mode Bits (Performance Enhance Indicator) */
    sCommand.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
    sCommand.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
    sCommand.AlternateBytes        = 0xFF;  /* Exit enhance mode */
    sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
    
    sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData             = 0;
    sCommand.DummyCycles        = DUMMY_CYCLES_READ_QUAD;           /* ★ v7: 4 cycles */
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
    
    if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* Memory-Mapped Mode 활성화 */
    sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;
    
    if (HAL_OSPI_MemoryMapped(&hospi1, &sMemMappedCfg) != HAL_OK)
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
    if (HAL_OSPI_Abort(&hospi1) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* DeInit 및 재초기화 */
    HAL_OSPI_DeInit(&hospi1);
    
    /* hospi1 초기화 */
    memset(&hospi1, 0, sizeof(hospi1));
    
    /* CubeMX 생성 함수로 재초기화 */
    MX_OCTOSPI1_Init();
    
    return HAL_OK;
}
