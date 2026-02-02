/**
 ******************************************************************************
 * @file    Loader_Src.c
 * @author  MCD Application Team (Modified for MX25L12833F + STM32U5G9)
 * @brief   This file defines the operations of the external loader for
 *          MX25L12833F SPI NOR memory on STM32U5G9ZJT6Q.
 *
 * 참고: MX25LM51245G_STM32U575I-EVAL 프로젝트 스타일 적용
 *
 * ★★★ 주요 변경 사항 ★★★
 * - MSI 오실레이터 사용 (HSE 대신) - External Loader 안정성 향상
 * - SysTick 우회 (HAL_InitTick, HAL_GetTick 오버라이드)
 * - Init()에서 SystemInit(), SystemClock_Config() 호출
 * - BSS 섹션 초기화 추가
 * - STM32U5G9 160MHz 지원
 ******************************************************************************
 */

#include "Loader_Src.h"
#include <string.h>

/* ============================================================================
 * MX25L12833F Commands (로컬 정의 - 헤더와 중복 방지)
 * ============================================================================ */
#define RESET_ENABLE_CMD_LOCAL               0x66
#define RESET_MEMORY_CMD_LOCAL               0x99
#define READ_ID_CMD_LOCAL                    0x9F
#define READ_CMD_LOCAL                       0x03
#define PAGE_PROG_CMD_LOCAL                  0x02
#define READ_STATUS_REG_CMD_LOCAL            0x05
#define WRITE_STATUS_REG_CMD_LOCAL           0x01
#define READ_CONFIG_REG_CMD_LOCAL            0x15
#define READ_SECURITY_REG_CMD_LOCAL          0x2B
#define WRITE_ENABLE_CMD_LOCAL               0x06
#define WRITE_DISABLE_CMD_LOCAL              0x04
#define SECTOR_ERASE_4K_CMD_LOCAL            0x20    /* 4KB Sector Erase */
#define BLOCK_ERASE_32K_CMD_LOCAL            0x52    /* 32KB Block Erase */
#define BLOCK_ERASE_64K_CMD_LOCAL            0xD8    /* 64KB Block Erase */
#define CHIP_ERASE_CMD_LOCAL                 0xC7
#define GANG_BLOCK_UNLOCK_CMD_LOCAL          0x98
#define EXIT_QPI_MODE_CMD                    0xF5    /* Exit QPI Mode */

/* Local Status Register Bit Masks */
#define STATUS_WIP                           0x01
#define STATUS_WEL                           0x02
#define STATUS_BP_MASK                       0x3C

/* Local Security Register Bit Masks */
#define SECURITY_P_FAIL                      0x20
#define SECURITY_E_FAIL                      0x40
#define SECURITY_WPSEL                       0x80

/* Local Memory Parameters */
//#define FLASH_SIZE                           0x01000000  /* 16MB */
#define SECTOR_SIZE                          0x1000      /* 4KB Sector */
#define BLOCK_SIZE                           0x10000     /* 64KB Block */
#define PAGE_SIZE                            0x100       /* 256 bytes */

/* Local Expected ID */
#define MANUFACTURER_ID                      0xC2
#define MEMORY_TYPE_ID                       0x20
#define MEMORY_DENSITY_ID                    0x18

/* Local Timeouts */
#define TIMEOUT_WRITE_REG                    100
#define TIMEOUT_PAGE_PROG                    10
#define TIMEOUT_SECTOR_ERASE                 300     /* 4KB Sector: max 120ms, 여유있게 300ms */
#define TIMEOUT_BLOCK_ERASE                  2000
#define TIMEOUT_CHIP_ERASE_LOCAL             120000

/* ============================================================================
 * OSPI Handle & 진단 변수
 * ============================================================================ */
static OSPI_HandleTypeDef hospi1_local;
static uint8_t g_flash_id[3] = {0, 0, 0};
static uint8_t g_init_error_code = 0;
static uint8_t g_status_reg = 0;
static uint8_t g_security_reg = 0;
static uint8_t g_last_erase_status = 0;
static uint8_t g_debug_status1 = 0;
static uint8_t g_debug_status2 = 0;

/* 디버그 변수 */
static uint32_t g_erase_input_start = 0;
static uint32_t g_erase_input_end = 0;
static uint32_t g_erase_actual_addr = 0;
static uint8_t g_erase_call_count = 0;
static uint8_t g_erase_loop_count = 0;
static uint8_t g_data_before_erase[4] = {0};
static uint8_t g_data_after_erase[4] = {0};
static uint8_t g_loader_version = 0x18;  /* v18: MSI 클럭 + 참고 프로젝트 스타일 */

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================ */
static void Loader_SystemClock_Config(void);
static void Loader_OCTOSPI1_Init(void);
static HAL_StatusTypeDef OSPI_WriteEnable(void);
static HAL_StatusTypeDef OSPI_AutoPollingMemReady(uint32_t Timeout);
static HAL_StatusTypeDef OSPI_ResetMemory(void);
static HAL_StatusTypeDef OSPI_QPI_Reset(void);
static HAL_StatusTypeDef OSPI_ReadID(uint8_t* id);
static HAL_StatusTypeDef OSPI_ReadStatusReg(uint8_t* status);
static HAL_StatusTypeDef OSPI_WriteStatusReg(uint8_t status);
static HAL_StatusTypeDef OSPI_ReadSecurityReg(uint8_t* security);
static HAL_StatusTypeDef OSPI_ClearBlockProtection(void);
static HAL_StatusTypeDef OSPI_GangBlockUnlock(void);
static HAL_StatusTypeDef OSPI_ManualWaitReady(uint32_t Timeout);
static HAL_StatusTypeDef OSPI_ReadBytes(uint32_t Address, uint8_t* buffer, uint32_t size);

/* ============================================================================
 * HAL Tick Override - Software Delay for External Loader
 * ============================================================================ */
/**
 * @brief  HAL_InitTick Override
 *         External Loader에서는 SysTick 인터럽트를 사용하지 않습니다.
 *         참고: MX25LM51245G_STM32U575I-EVAL 프로젝트
 */
KeepInCompilation HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    (void)TickPriority;
    return HAL_OK;
}

/**
 * @brief  HAL_GetTick Override
 *         참고 프로젝트에서는 항상 1을 반환합니다.
 *         HAL 타임아웃 체크에서 0을 반환하면 문제가 발생할 수 있습니다.
 */
uint32_t HAL_GetTick(void)
{
    return 1;  /* 참고 프로젝트: return 1 */
}

/**
 * @brief  HAL_Delay Override
 *         External Loader에서는 SysTick을 사용하지 않으므로
 *         소프트웨어 딜레이로 대체합니다.
 *         160MHz 기준으로 대략 1ms 딜레이
 */
void HAL_Delay(uint32_t Delay)
{
    volatile uint32_t i;
    /* 160MHz 기준으로 대략 1ms 딜레이 */
    for (i = 0; i < (Delay * 32000); i++)
    {
        __NOP();
    }
}

/* ============================================================================
 * OSPI_ReadBytes - 내부 Read 함수
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ReadBytes(uint32_t Address, uint8_t* buffer, uint32_t size)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = READ_CMD_LOCAL;
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
    sCommand.NbData             = size;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_OSPI_Receive(&hospi1_local, buffer, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ============================================================================
 * OSPI_ReadID
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ReadID(uint8_t* id)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = READ_ID_CMD_LOCAL;
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
    sCommand.Instruction        = READ_STATUS_REG_CMD_LOCAL;
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
 * OSPI_WriteStatusReg
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_WriteStatusReg(uint8_t status)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    if (OSPI_WriteEnable() != HAL_OK)
    {
        return HAL_ERROR;
    }

    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = WRITE_STATUS_REG_CMD_LOCAL;
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

    if (HAL_OSPI_Transmit(&hospi1_local, &status, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (OSPI_AutoPollingMemReady(TIMEOUT_WRITE_REG) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ============================================================================
 * OSPI_ReadSecurityReg
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ReadSecurityReg(uint8_t* security)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = READ_SECURITY_REG_CMD_LOCAL;
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

    if (HAL_OSPI_Receive(&hospi1_local, security, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ============================================================================
 * OSPI_ClearBlockProtection
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ClearBlockProtection(void)
{
    uint8_t status_reg = 0;
    uint8_t new_status = 0;

    if (OSPI_ReadStatusReg(&status_reg) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if ((status_reg & STATUS_BP_MASK) == 0)
    {
        return HAL_OK;
    }

    new_status = status_reg & ~STATUS_BP_MASK;

    if (OSPI_WriteStatusReg(new_status) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (OSPI_ReadStatusReg(&status_reg) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if ((status_reg & STATUS_BP_MASK) != 0)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ============================================================================
 * OSPI_GangBlockUnlock
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_GangBlockUnlock(void)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    if (OSPI_WriteEnable() != HAL_OK)
    {
        return HAL_ERROR;
    }

    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = GANG_BLOCK_UNLOCK_CMD_LOCAL;
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

    if (OSPI_AutoPollingMemReady(TIMEOUT_WRITE_REG) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ============================================================================
 * OSPI_ManualWaitReady
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ManualWaitReady(uint32_t Timeout)
{
    uint8_t status = 0;
    volatile uint32_t count = 0;
    uint32_t max_count = Timeout * 1000;  /* 대략적인 카운트 */

    do
    {
        if (OSPI_ReadStatusReg(&status) != HAL_OK)
        {
            return HAL_ERROR;
        }

        if ((status & STATUS_WIP) == 0)
        {
            return HAL_OK;
        }

        for (volatile uint32_t i = 0; i < 100; i++) { __NOP(); }
        count++;

    } while (count < max_count);

    return HAL_TIMEOUT;
}

/* ============================================================================
 * OSPI_QPI_Reset - QPI 모드에서 SPI 모드로 복귀
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_QPI_Reset(void)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    /* Exit QPI Mode (0xF5) - 4-line instruction */
    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = EXIT_QPI_MODE_CMD;
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_4_LINES;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles        = 0;
    sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

    /* 실패해도 무시 (이미 SPI 모드일 수 있음) */
    HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

    HAL_Delay(1);

    return HAL_OK;
}

/* ============================================================================
 * Init
 * ============================================================================
 * @brief  System initialization.
 *         참고: MX25LM51245G_STM32U575I-EVAL 프로젝트 스타일 적용
 * @retval  1 : Operation succeeded
 * @retval  0 : Operation failed
 */
KeepInCompilation int Init(void)
{
    /* ========================================================================
     * 1. BSS 섹션 초기화 (참고 프로젝트 스타일)
     *    External Loader는 일반 어플리케이션과 다르게 초기화가 필요합니다.
     * ======================================================================== */
#if defined(__ICCARM__)
    /* IAR 컴파일러용 BSS 초기화 */
    char *startadd = __section_begin(".bss");
    uint32_t size = __section_size(".bss");
    memset(startadd, 0, size);
#else
    /* GCC 컴파일러용 BSS 초기화 */
    extern uint32_t _sbss, _ebss;
    uint32_t *pBss = &_sbss;
    while (pBss < &_ebss)
    {
        *pBss++ = 0;
    }
#endif

    /* 디버그 변수 초기화 */
    g_init_error_code = 0;
    g_flash_id[0] = 0;
    g_flash_id[1] = 0;
    g_flash_id[2] = 0;
    g_status_reg = 0;
    g_security_reg = 0;
    g_last_erase_status = 0;
    g_debug_status1 = 0;
    g_debug_status2 = 0;
    g_erase_input_start = 0;
    g_erase_input_end = 0;
    g_erase_actual_addr = 0;
    g_erase_call_count = 0;
    g_erase_loop_count = 0;
    memset(g_data_before_erase, 0, 4);
    memset(g_data_after_erase, 0, 4);
    g_loader_version = 0x18;  /* v18: MSI 클럭 + 참고 프로젝트 스타일 */

    memset(&hospi1_local, 0, sizeof(hospi1_local));

    /* ========================================================================
     * 2. 시스템 초기화 (참고 프로젝트 스타일)
     * ======================================================================== */
    SystemInit();
    HAL_Init();

    /* ========================================================================
     * 3. 클럭 설정 (MSI + PLL → 160MHz)
     * ======================================================================== */
    Loader_SystemClock_Config();

    /* ========================================================================
     * 4. OCTOSPI 초기화
     * ======================================================================== */
    Loader_OCTOSPI1_Init();

    HAL_Delay(100);

    /* QPI Reset 시도 (만약 Flash가 QPI 모드라면 SPI 모드로 복귀) */
    OSPI_QPI_Reset();

    HAL_Delay(50);

    /* Memory Reset */
    if (OSPI_ResetMemory() != HAL_OK)
    {
        g_init_error_code = 1;
        return 0;
    }

    HAL_Delay(100);

    /* Read ID */
    if (OSPI_ReadID(g_flash_id) != HAL_OK)
    {
        g_init_error_code = 2;
        return 0;
    }

    /* Read Status Register */
    if (OSPI_ReadStatusReg(&g_status_reg) != HAL_OK)
    {
        g_init_error_code = 3;
        return 0;
    }

    /* Read Security Register */
    if (OSPI_ReadSecurityReg(&g_security_reg) != HAL_OK)
    {
        g_init_error_code = 6;
    }

    /* ID 확인 */
    if (g_flash_id[0] != MANUFACTURER_ID)
    {
        g_init_error_code = 4;
    }

    /* Block Protection 해제 */
    if (g_security_reg & SECURITY_WPSEL)
    {
        if (OSPI_GangBlockUnlock() != HAL_OK)
        {
            g_init_error_code = 7;
        }
    }
    else
    {
        if (OSPI_ClearBlockProtection() != HAL_OK)
        {
            g_init_error_code = 5;
        }
    }

    /* 최종 상태 확인 */
    OSPI_ReadStatusReg(&g_status_reg);
    OSPI_ReadSecurityReg(&g_security_reg);

    return 1;
}

/* ============================================================================
 * Read
 * ============================================================================ */
KeepInCompilation int Read(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    /* 디버그 주소 처리 */
    if (Address == 0x90000090)
    {
        OSPI_ReadStatusReg(&g_status_reg);
        OSPI_ReadSecurityReg(&g_security_reg);

        if (Size >= 32)
        {
            buffer[0] = g_flash_id[0];
            buffer[1] = g_flash_id[1];
            buffer[2] = g_flash_id[2];
            buffer[3] = g_init_error_code;
            buffer[4] = g_status_reg;
            buffer[5] = g_security_reg;
            buffer[6] = (g_status_reg & STATUS_BP_MASK) >> 2;
            buffer[7] = g_last_erase_status;
            buffer[8] = 0xAA;
            buffer[9] = 0x55;
            buffer[10] = (g_security_reg & SECURITY_WPSEL) ? 1 : 0;
            buffer[11] = (g_security_reg & SECURITY_E_FAIL) ? 1 : 0;
            buffer[12] = g_debug_status1;
            buffer[13] = g_debug_status2;
            buffer[14] = g_erase_call_count;
            buffer[15] = g_erase_loop_count;

            buffer[16] = (uint8_t)(g_erase_input_start & 0xFF);
            buffer[17] = (uint8_t)((g_erase_input_start >> 8) & 0xFF);
            buffer[18] = (uint8_t)((g_erase_input_start >> 16) & 0xFF);
            buffer[19] = (uint8_t)((g_erase_input_start >> 24) & 0xFF);

            buffer[20] = (uint8_t)(g_erase_input_end & 0xFF);
            buffer[21] = (uint8_t)((g_erase_input_end >> 8) & 0xFF);
            buffer[22] = (uint8_t)((g_erase_input_end >> 16) & 0xFF);
            buffer[23] = (uint8_t)((g_erase_input_end >> 24) & 0xFF);

            buffer[24] = (uint8_t)(g_erase_actual_addr & 0xFF);
            buffer[25] = (uint8_t)((g_erase_actual_addr >> 8) & 0xFF);
            buffer[26] = (uint8_t)((g_erase_actual_addr >> 16) & 0xFF);
            buffer[27] = (uint8_t)((g_erase_actual_addr >> 24) & 0xFF);

            buffer[28] = g_data_after_erase[0];
            buffer[29] = g_data_after_erase[1];
            buffer[30] = g_data_after_erase[2];
            buffer[31] = g_data_after_erase[3];
        }
        else if (Size >= 16)
        {
            buffer[0] = g_flash_id[0];
            buffer[1] = g_flash_id[1];
            buffer[2] = g_flash_id[2];
            buffer[3] = g_init_error_code;
            buffer[4] = g_status_reg;
            buffer[5] = g_security_reg;
            buffer[6] = (g_status_reg & STATUS_BP_MASK) >> 2;
            buffer[7] = g_last_erase_status;
            buffer[8] = 0xAA;
            buffer[9] = 0x55;
            buffer[10] = (g_security_reg & SECURITY_WPSEL) ? 1 : 0;
            buffer[11] = (g_security_reg & SECURITY_E_FAIL) ? 1 : 0;
            buffer[12] = g_debug_status1;
            buffer[13] = g_debug_status2;
            buffer[14] = g_erase_call_count;
            buffer[15] = g_erase_loop_count;
        }
        return 1;
    }

    if (Address == 0x90000080)
    {
        if (Size >= 4)
        {
            buffer[0] = g_loader_version;
            buffer[1] = g_data_before_erase[0];
            buffer[2] = g_data_before_erase[1];
            buffer[3] = g_data_before_erase[2];
        }
        return 1;
    }

    /* 주소 변환 */
    if (Address >= 0x90000000)
    {
        Address -= 0x90000000;
    }

    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = READ_CMD_LOCAL;
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

    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    if (HAL_OSPI_Receive(&hospi1_local, buffer, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 0;
    }

    return 1;
}

/* ============================================================================
 * Write
 * ============================================================================ */
KeepInCompilation int Write(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint32_t end_addr, current_size, current_addr;

    if (Address >= 0x90000000)
    {
        Address -= 0x90000000;
    }

    current_addr = Address;
    end_addr = Address + Size;

    while (current_addr < end_addr)
    {
        current_size = PAGE_SIZE - (current_addr % PAGE_SIZE);
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
        sCommand.Instruction        = PAGE_PROG_CMD_LOCAL;
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

        if (OSPI_AutoPollingMemReady(TIMEOUT_PAGE_PROG) != HAL_OK)
        {
            return 0;
        }

        current_addr += current_size;
        buffer += current_size;
    }

    return 1;
}

/* ============================================================================
 * SectorErase - ★★★ 4KB Sector Erase (0x20) ★★★
 *
 * Dev_Inf.c와 일치: 4096 Sectors × 4KB = 16MB
 * ============================================================================ */
KeepInCompilation int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    OSPI_RegularCmdTypeDef sCommand;
    uint32_t SectorAddr;
    uint8_t status_before = 0;
    uint8_t status_after = 0;

    /* 디버그: 입력 주소 저장 */
    g_erase_input_start = EraseStartAddress;
    g_erase_input_end = EraseEndAddress;
    g_erase_call_count++;
    g_erase_loop_count = 0;

    g_last_erase_status = 0;
    g_debug_status1 = 0;
    g_debug_status2 = 0;

    /* 주소 변환 */
    if (EraseStartAddress >= 0x90000000)
    {
        EraseStartAddress -= 0x90000000;
    }
    if (EraseEndAddress >= 0x90000000)
    {
        EraseEndAddress -= 0x90000000;
    }

    /* 4KB Sector 경계로 정렬 */
    EraseStartAddress = EraseStartAddress - (EraseStartAddress % SECTOR_SIZE);

    while (EraseEndAddress >= EraseStartAddress)
    {
        g_erase_loop_count++;
        SectorAddr = EraseStartAddress;
        g_erase_actual_addr = SectorAddr;

        /* Erase 전 데이터 읽기 */
        OSPI_ReadBytes(SectorAddr, g_data_before_erase, 4);

        /* 1. Write Enable */
        memset(&sCommand, 0, sizeof(OSPI_RegularCmdTypeDef));

        sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
        sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
        sCommand.Instruction        = WRITE_ENABLE_CMD_LOCAL;
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
            g_last_erase_status = 0x10;
            return 0;
        }

        HAL_Delay(1);

        if (OSPI_ReadStatusReg(&status_before) != HAL_OK)
        {
            g_last_erase_status = 0x11;
            return 0;
        }

        g_debug_status2 = status_before;

        if ((status_before & STATUS_WEL) == 0)
        {
            g_last_erase_status = 0x12;
            return 0;
        }

        /* 2. 4KB Sector Erase Command (0x20) */
        memset(&sCommand, 0, sizeof(OSPI_RegularCmdTypeDef));

        sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
        sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
        sCommand.Instruction        = SECTOR_ERASE_4K_CMD_LOCAL;  /* 0x20 */
        sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
        sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
        sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
        sCommand.Address            = SectorAddr;
        sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
        sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
        sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
        sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
        sCommand.DataMode           = HAL_OSPI_DATA_NONE;
        sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
        sCommand.NbData             = 0;
        sCommand.DummyCycles        = 0;
        sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
        sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

        if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            g_last_erase_status = 0x20;
            return 0;
        }

        /* 3. Erase 명령 직후 상태 저장 */
        HAL_Delay(1);
        OSPI_ReadStatusReg(&g_debug_status1);

        /* 4. Erase 완료 대기 (4KB는 최대 120ms) */
        HAL_Delay(10);

        if (OSPI_ManualWaitReady(TIMEOUT_SECTOR_ERASE) != HAL_OK)
        {
            g_last_erase_status = 0x30;
            return 0;
        }

        /* 5. 완료 후 상태 확인 */
        if (OSPI_ReadStatusReg(&status_after) != HAL_OK)
        {
            g_last_erase_status = 0x40;
            return 0;
        }

        if (status_after & STATUS_WEL)
        {
            g_last_erase_status = 0x41;
        }

        OSPI_ReadSecurityReg(&g_security_reg);
        if (g_security_reg & SECURITY_E_FAIL)
        {
            g_last_erase_status = 0x50;
            return 0;
        }

        /* Erase 후 데이터 읽기 */
        OSPI_ReadBytes(SectorAddr, g_data_after_erase, 4);

        /* 4KB 단위로 증가 */
        EraseStartAddress += SECTOR_SIZE;
    }

    return 1;
}

/* ============================================================================
 * MassErase
 * ============================================================================ */
KeepInCompilation int MassErase(uint32_t Parallelism)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint8_t status = 0;
    (void)Parallelism;

    g_last_erase_status = 0;

    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = WRITE_ENABLE_CMD_LOCAL;
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
        g_last_erase_status = 0x10;
        return 0;
    }

    HAL_Delay(1);
    if (OSPI_ReadStatusReg(&status) != HAL_OK)
    {
        g_last_erase_status = 0x11;
        return 0;
    }

    if ((status & STATUS_WEL) == 0)
    {
        g_last_erase_status = 0x12;
        return 0;
    }

    memset(&sCommand, 0, sizeof(OSPI_RegularCmdTypeDef));

    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = CHIP_ERASE_CMD_LOCAL;
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
        g_last_erase_status = 0x20;
        return 0;
    }

    HAL_Delay(1000);

    if (OSPI_ManualWaitReady(TIMEOUT_CHIP_ERASE_LOCAL) != HAL_OK)
    {
        g_last_erase_status = 0x30;
        return 0;
    }

    OSPI_ReadSecurityReg(&g_security_reg);
    if (g_security_reg & SECURITY_E_FAIL)
    {
        g_last_erase_status = 0x50;
        return 0;
    }

    return 1;
}

/* ============================================================================
 * Verify
 * ============================================================================ */
KeepInCompilation uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr,
                                   uint32_t Size, uint32_t missalignement)
{
    uint32_t VerifiedData = 0;
    uint32_t InitialOffset = (missalignement & 0xF);
    uint8_t* pFlash = (uint8_t*)(MemoryAddr);
    uint8_t* pRAM = (uint8_t*)(RAMBufferAddr);
    uint8_t flashByte;
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint32_t flashAddr;

    pRAM += InitialOffset;
    pFlash += InitialOffset;

    while (VerifiedData < (Size - InitialOffset))
    {
        flashAddr = (uint32_t)pFlash;
        if (flashAddr >= 0x90000000)
        {
            flashAddr -= 0x90000000;
        }

        sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
        sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
        sCommand.Instruction        = READ_CMD_LOCAL;
        sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
        sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
        sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
        sCommand.Address            = flashAddr;
        sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
        sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
        sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
        sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
        sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
        sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
        sCommand.NbData             = 1;
        sCommand.DummyCycles        = 0;
        sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
        sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

        if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return (uint64_t)((uint32_t)pFlash);
        }

        if (HAL_OSPI_Receive(&hospi1_local, &flashByte, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            return (uint64_t)((uint32_t)pFlash);
        }

        if (flashByte != *pRAM)
        {
            return (uint64_t)((uint32_t)pFlash);
        }

        pFlash++;
        pRAM++;
        VerifiedData++;
    }

    return (uint64_t)(MemoryAddr + Size - InitialOffset);
}

/* ============================================================================
 * Loader_SystemClock_Config
 * ============================================================================
 * @brief  System Clock Configuration
 *         참고: MX25LM51245G_STM32U575I-EVAL 프로젝트 스타일 적용
 *
 *         ★★★ MSI 오실레이터 사용 (HSE 대신) ★★★
 *         - External Loader에서는 외부 크리스탈(HSE)이 안정적으로
 *           동작하지 않을 수 있으므로 내부 오실레이터(MSI) 사용
 *         - STM32U5G9ZJT6Q 최대 클럭: 160MHz
 *
 *         클럭 계산:
 *         - MSI = 4MHz (RCC_MSIRANGE_4)
 *         - PLL: 4MHz × 80 / 2 = 160MHz SYSCLK
 *
 * @retval None
 */
static void Loader_SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

    /* Enable voltage range 1 for frequency above 100 Mhz */
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    __HAL_RCC_PWR_CLK_DISABLE();

    /* ========================================================================
     * MSI Oscillator Configuration (참고 프로젝트 스타일)
     * - MSI는 리셋 후 기본적으로 활성화됨 (4MHz)
     * - PLL 소스로 MSI 사용 → 외부 크리스탈 불필요
     * ======================================================================== */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;  /* 4MHz */
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;  /* PLL 소스 = MSI */
    RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
    RCC_OscInitStruct.PLL.PLLM = 1;    /* VCO input = 4MHz / 1 = 4MHz */
    RCC_OscInitStruct.PLL.PLLN = 80;   /* VCO output = 4MHz × 80 = 320MHz */
    RCC_OscInitStruct.PLL.PLLR = 2;    /* SYSCLK = 320MHz / 2 = 160MHz */
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        /* Initialization Error */
        while(1);
    }

    /* ========================================================================
     * Bus Clock Configuration
     * - SYSCLK = 160MHz
     * - HCLK = 160MHz (AHB)
     * - PCLK1/2/3 = 160MHz (APB1/2/3)
     * ======================================================================== */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                   RCC_CLOCKTYPE_PCLK3);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        /* Initialization Error */
        while(1);
    }
}

/* ============================================================================
 * Loader_OCTOSPI1_Init
 * ============================================================================
 * @brief  OCTOSPI1 초기화
 *
 *         OSPI 클럭 계산:
 *         - SYSCLK = 160MHz (MSI 4MHz × 80 / 2)
 *         - ClockPrescaler = 8
 *         - OSPI Clock = 160MHz / 8 = 20MHz
 *
 *         MX25L12833F 최대 SPI 클럭: 133MHz
 *         → 20MHz는 충분히 안전한 속도
 */
static void Loader_OCTOSPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    OSPIM_CfgTypeDef sOspiManagerCfg = {0};

    /* Enable clocks */
    __HAL_RCC_OSPIM_CLK_ENABLE();
    __HAL_RCC_OSPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* GPIO Configuration */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* OSPI Configuration */
    hospi1_local.Instance = OCTOSPI1;
    hospi1_local.Init.FifoThreshold = 4;
    hospi1_local.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
    hospi1_local.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;  /* Macronix 플래시 */
    hospi1_local.Init.DeviceSize = 24;  /* 2^24 = 16MB */
    hospi1_local.Init.ChipSelectHighTime = 8;
    hospi1_local.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
    hospi1_local.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
    hospi1_local.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
    hospi1_local.Init.ClockPrescaler = 8;  /* 160MHz / 8 = 20MHz */
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
    sCommand.Instruction        = WRITE_ENABLE_CMD_LOCAL;
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

    sCommand.Instruction = READ_STATUS_REG_CMD_LOCAL;
    sCommand.DataMode    = HAL_OSPI_DATA_1_LINE;
    sCommand.NbData      = 1;
    sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;

    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    sConfig.Match         = STATUS_WEL;
    sConfig.Mask          = STATUS_WEL;
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
    sCommand.Instruction        = READ_STATUS_REG_CMD_LOCAL;
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
    sConfig.Mask          = STATUS_WIP;
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
    sCommand.Instruction        = RESET_ENABLE_CMD_LOCAL;
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

    sCommand.Instruction = RESET_MEMORY_CMD_LOCAL;

    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    HAL_Delay(30);

    return HAL_OK;
}
