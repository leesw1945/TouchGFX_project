/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @author  Fixed Version v13 - Block Protection 해제 추가
  * @brief   MX25L12833F External Loader (SPI 1-Line Mode)
  * 
  * ★★★ 수정 사항 (v13) ★★★
  * - Block Protection (BP) 비트 해제 기능 추가
  * - Security Register 읽기 기능 추가 (E_FAIL 진단용)
  * - Write Status Register 기능 추가
  * - 진단 정보 확장 (BP 상태, Security Register)
  * - Erase 타임아웃 값 조정
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
#define PAGE_PROG_CMD                        0x02    /* Page Program (1-1-1) */
#define READ_STATUS_REG_CMD                  0x05    /* Read Status Register */
#define WRITE_STATUS_REG_CMD                 0x01    /* Write Status Register */
#define READ_CONFIG_REG_CMD                  0x15    /* Read Configuration Register */
#define READ_SECURITY_REG_CMD                0x2B    /* Read Security Register (RDSCUR) */
#define WRITE_ENABLE_CMD                     0x06
#define WRITE_DISABLE_CMD                    0x04
#define SECTOR_ERASE_4K_CMD                  0x20    /* Sector Erase 4KB */
#define BLOCK_ERASE_32K_CMD                  0x52    /* Block Erase 32KB */
#define BLOCK_ERASE_64K_CMD                  0xD8    /* Block Erase 64KB */
#define CHIP_ERASE_CMD                       0xC7
#define GANG_BLOCK_UNLOCK_CMD                0x98    /* GBULK - Unprotect all blocks */

/* Status Register Bit Masks */
#define STATUS_REG_WIP_MASK                  0x01    /* bit0: Write In Progress */
#define STATUS_REG_WEL_MASK                  0x02    /* bit1: Write Enable Latch */
#define STATUS_REG_BP0_MASK                  0x04    /* bit2: Block Protect 0 */
#define STATUS_REG_BP1_MASK                  0x08    /* bit3: Block Protect 1 */
#define STATUS_REG_BP2_MASK                  0x10    /* bit4: Block Protect 2 */
#define STATUS_REG_BP3_MASK                  0x20    /* bit5: Block Protect 3 */
#define STATUS_REG_QE_MASK                   0x40    /* bit6: Quad Enable */
#define STATUS_REG_SRWD_MASK                 0x80    /* bit7: Status Register Write Disable */
#define STATUS_REG_BP_MASK                   0x3C    /* bit5-2: All BP bits */

/* Security Register Bit Masks */
#define SECURITY_REG_OTP_LOCK_MASK           0x03    /* bit1-0: OTP Lock */
#define SECURITY_REG_LDSO_MASK               0x02    /* bit1: Lock-down Secured OTP */
#define SECURITY_REG_PSB_MASK                0x04    /* bit2: Program Suspend */
#define SECURITY_REG_ESB_MASK                0x08    /* bit3: Erase Suspend */
#define SECURITY_REG_P_FAIL_MASK             0x20    /* bit5: Program Fail */
#define SECURITY_REG_E_FAIL_MASK             0x40    /* bit6: Erase Fail */
#define SECURITY_REG_WPSEL_MASK              0x80    /* bit7: Write Protect Selection */

/* Memory Parameters */
#define MEMORY_FLASH_SIZE                    0x01000000  /* 16MB */
#define MEMORY_BLOCK_SIZE                    0x10000     /* 64KB */
#define MEMORY_SECTOR_SIZE                   0x1000      /* 4KB */
#define MEMORY_PAGE_SIZE                     0x100       /* 256 bytes */

/* Expected ID */
#define MX25L12833F_MANUFACTURER_ID          0xC2
#define MX25L12833F_MEMORY_TYPE              0x20
#define MX25L12833F_MEMORY_DENSITY           0x18

/* Timeouts (in ms) - 데이터시트 기반 */
#define TIMEOUT_WRITE_STATUS_REG             100     /* tW: max 40ms */
#define TIMEOUT_PAGE_PROGRAM                 10      /* tPP: max 1.2ms */
#define TIMEOUT_SECTOR_ERASE_4K              200     /* tSE: max 120ms */
#define TIMEOUT_BLOCK_ERASE_32K              1000    /* tBE32: max 650ms */
#define TIMEOUT_BLOCK_ERASE_64K              1000    /* tBE: max 650ms */
#define TIMEOUT_CHIP_ERASE                   120000  /* tCE: max 60s (여유있게 120s) */

/* ============================================================================
 * OSPI Handle & 진단 변수
 * ============================================================================ */
static OSPI_HandleTypeDef hospi1_local;
static uint8_t g_flash_id[3] = {0, 0, 0};
static uint8_t g_init_error_code = 0;
static uint8_t g_status_reg = 0;
static uint8_t g_security_reg = 0;
static uint8_t g_last_erase_status = 0;

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
static HAL_StatusTypeDef OSPI_WriteStatusReg(uint8_t status);
static HAL_StatusTypeDef OSPI_ReadSecurityReg(uint8_t* security);
static HAL_StatusTypeDef OSPI_ClearBlockProtection(void);
static HAL_StatusTypeDef OSPI_GangBlockUnlock(void);

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
 * OSPI_ReadID - Flash ID 읽기 (RDID: 0x9F)
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
 * OSPI_ReadStatusReg - Status Register 읽기 (RDSR: 0x05)
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
 * OSPI_WriteStatusReg - Status Register 쓰기 (WRSR: 0x01)
 *
 * MX25L12833F는 Status Register만 쓰기 (1바이트)
 * BP3-BP0 비트를 0으로 설정하여 Block Protection 해제
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_WriteStatusReg(uint8_t status)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    /* Write Enable 먼저 필요 */
    if (OSPI_WriteEnable() != HAL_OK)
    {
        return HAL_ERROR;
    }

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

    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_OSPI_Transmit(&hospi1_local, &status, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Write Status Register 완료 대기 (최대 40ms) */
    if (OSPI_AutoPollingMemReady(TIMEOUT_WRITE_STATUS_REG) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ============================================================================
 * OSPI_ReadSecurityReg - Security Register 읽기 (RDSCUR: 0x2B)
 *
 * E_FAIL, P_FAIL 비트로 erase/program 실패 원인 진단
 * WPSEL 비트로 보호 모드 확인
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ReadSecurityReg(uint8_t* security)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = READ_SECURITY_REG_CMD;
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
 * OSPI_ClearBlockProtection - Block Protection 비트 해제
 *
 * Status Register의 BP3-BP0 비트를 0으로 설정하여 전체 메모리 보호 해제
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ClearBlockProtection(void)
{
    uint8_t status_reg = 0;
    uint8_t new_status = 0;

    /* 1. 현재 Status Register 읽기 */
    if (OSPI_ReadStatusReg(&status_reg) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* 2. BP 비트가 이미 0이면 성공 리턴 */
    if ((status_reg & STATUS_REG_BP_MASK) == 0)
    {
        return HAL_OK;  /* 이미 보호 해제됨 */
    }

    /* 3. BP 비트만 클리어 (다른 비트 유지, QE 비트 유지) */
    new_status = status_reg & ~STATUS_REG_BP_MASK;

    /* 4. Write Status Register */
    if (OSPI_WriteStatusReg(new_status) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* 5. 확인 */
    if (OSPI_ReadStatusReg(&status_reg) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if ((status_reg & STATUS_REG_BP_MASK) != 0)
    {
        return HAL_ERROR;  /* 여전히 보호됨 - 실패 */
    }

    return HAL_OK;
}

/* ============================================================================
 * OSPI_GangBlockUnlock - 전체 블록 언락 (GBULK: 0x98)
 *
 * WPSEL=1 (Individual Sector Protection 모드)인 경우에 사용
 * 모든 SPB(Solid Protection Bit)를 한 번에 해제
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_GangBlockUnlock(void)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    /* Write Enable 먼저 필요 */
    if (OSPI_WriteEnable() != HAL_OK)
    {
        return HAL_ERROR;
    }

    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = GANG_BLOCK_UNLOCK_CMD;
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

    /* 완료 대기 */
    if (OSPI_AutoPollingMemReady(TIMEOUT_WRITE_STATUS_REG) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ============================================================================
 * Init - System Initialization
 *
 * Error Codes:
 *   0 = No error
 *   1 = Reset failed
 *   2 = Read ID failed
 *   3 = Read Status Register failed
 *   4 = ID mismatch (warning only)
 *   5 = Clear Block Protection failed
 *   6 = Read Security Register failed
 *   7 = Gang Block Unlock failed (WPSEL=1 mode)
 * ============================================================================ */
__attribute__((used)) int Init(void)
{
    g_init_error_code = 0;
    g_flash_id[0] = 0;
    g_flash_id[1] = 0;
    g_flash_id[2] = 0;
    g_status_reg = 0;
    g_security_reg = 0;
    g_last_erase_status = 0;

    memset(&hospi1_local, 0, sizeof(hospi1_local));
    uwTick_local = 0;

    HAL_Init();
    Loader_SystemClock_Config();
    Loader_OCTOSPI1_Init();
    
    /* 충분한 안정화 시간 */
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

    /* Security Register 읽기 (진단용) */
    if (OSPI_ReadSecurityReg(&g_security_reg) != HAL_OK)
    {
        g_init_error_code = 6;
        /* 계속 진행 - 필수 아님 */
    }

    /* ID 검증 */
    if (g_flash_id[0] != MX25L12833F_MANUFACTURER_ID)
    {
        g_init_error_code = 4;  /* ID 불일치 - 경고만 (계속 진행) */
    }

    /* ★★★ Block Protection 해제 ★★★ */
    /* WPSEL 비트 확인 (Security Register bit7) */
    if (g_security_reg & SECURITY_REG_WPSEL_MASK)
    {
        /* WPSEL=1: Individual Sector Protection 모드 */
        /* GBULK 명령으로 모든 SPB 해제 */
        if (OSPI_GangBlockUnlock() != HAL_OK)
        {
            g_init_error_code = 7;
            /* 계속 진행 시도 */
        }
    }
    else
    {
        /* WPSEL=0: Block Protection (BP) 모드 (기본값) */
        /* Status Register의 BP 비트 해제 */
        if (OSPI_ClearBlockProtection() != HAL_OK)
        {
            g_init_error_code = 5;
            /* 계속 진행 시도 */
        }
    }

    /* 최종 상태 업데이트 */
    OSPI_ReadStatusReg(&g_status_reg);
    OSPI_ReadSecurityReg(&g_security_reg);

    return 1;
}

/* ============================================================================
 * Read - Indirect Mode에서 직접 읽기 (0x03 Normal Read)
 *
 * ★ 진단 주소 ★
 * 0x90000090: Flash ID + Status 반환 (16 bytes)
 *   [0]  = Manufacturer ID (0xC2)
 *   [1]  = Memory Type (0x20)
 *   [2]  = Density (0x18)
 *   [3]  = Init Error Code
 *   [4]  = Status Register (현재)
 *   [5]  = Security Register (현재)
 *   [6]  = Status Register BP bits only ((SR >> 2) & 0x0F)
 *   [7]  = Last Erase Status:
 *          0x00 = OK
 *          0x10 = WREN command fail
 *          0x11 = Read status fail after WREN
 *          0x12 = WEL not set after WREN
 *          0x20 = Erase command fail
 *          0x30 = Timeout waiting for WIP=0
 *          0x40 = Read status fail after erase
 *          0x41 = WEL still set after erase (command ignored?)
 *          0x50 = E_FAIL set in security register
 *   [8]  = 0xAA (마커)
 *   [9]  = 0x55 (마커)
 *   [10] = WPSEL bit (0 or 1)
 *   [11] = E_FAIL bit (0 or 1)
 *   [12-15] = Reserved (0x00)
 * ============================================================================ */
__attribute__((used)) int Read(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    /* 진단 주소 */
    if (Address == 0x90000090)
    {
        /* 최신 상태 읽기 */
        OSPI_ReadStatusReg(&g_status_reg);
        OSPI_ReadSecurityReg(&g_security_reg);

        if (Size >= 16)
        {
            buffer[0] = g_flash_id[0];                              /* Manufacturer ID */
            buffer[1] = g_flash_id[1];                              /* Memory Type */
            buffer[2] = g_flash_id[2];                              /* Density */
            buffer[3] = g_init_error_code;                          /* Init Error Code */
            buffer[4] = g_status_reg;                               /* Status Register */
            buffer[5] = g_security_reg;                             /* Security Register */
            buffer[6] = (g_status_reg & STATUS_REG_BP_MASK) >> 2;   /* BP bits only */
            buffer[7] = g_last_erase_status;                        /* Last Erase Status */
            buffer[8] = 0xAA;                                       /* 마커 */
            buffer[9] = 0x55;                                       /* 마커 */
            buffer[10] = (g_security_reg & SECURITY_REG_WPSEL_MASK) ? 1 : 0;  /* WPSEL */
            buffer[11] = (g_security_reg & SECURITY_REG_E_FAIL_MASK) ? 1 : 0; /* E_FAIL */
            buffer[12] = 0x00;
            buffer[13] = 0x00;
            buffer[14] = 0x00;
            buffer[15] = 0x00;
        }
        else if (Size >= 8)
        {
            buffer[0] = g_flash_id[0];
            buffer[1] = g_flash_id[1];
            buffer[2] = g_flash_id[2];
            buffer[3] = g_init_error_code;
            buffer[4] = g_status_reg;
            buffer[5] = 0xAA;
            buffer[6] = 0x55;
            buffer[7] = 0x00;
        }
        return 1;
    }

    /* 주소 변환 */
    if (Address >= 0x90000000)
    {
        Address -= 0x90000000;
    }

    /* Normal Read (0x03) - 1-1-1 모드 */
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
 * Write - Page Program (0x02) - 1-1-1 모드
 * ============================================================================ */
__attribute__((used)) int Write(uint32_t Address, uint32_t Size, uint8_t* buffer)
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
        current_size = MEMORY_PAGE_SIZE - (current_addr % MEMORY_PAGE_SIZE);
        if (current_size > (end_addr - current_addr))
        {
            current_size = end_addr - current_addr;
        }
        
        if (OSPI_WriteEnable() != HAL_OK)
        {
            return 0;
        }
        
        /* Page Program (0x02) - 1-1-1 모드 */
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
        
        if (OSPI_AutoPollingMemReady(TIMEOUT_PAGE_PROGRAM) != HAL_OK)
        {
            return 0;
        }
        
        current_addr += current_size;
        buffer += current_size;
    }
    
    return 1;
}

/* ============================================================================
 * OSPI_ManualWaitReady - 수동으로 WIP 비트 폴링 (AutoPolling 대신)
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ManualWaitReady(uint32_t Timeout)
{
    uint8_t status = 0;
    uint32_t tickstart = HAL_GetTick();

    do
    {
        if (OSPI_ReadStatusReg(&status) != HAL_OK)
        {
            return HAL_ERROR;
        }

        /* WIP = 0 이면 완료 */
        if ((status & STATUS_REG_WIP_MASK) == 0)
        {
            return HAL_OK;
        }

        HAL_Delay(1);

    } while ((HAL_GetTick() - tickstart) < Timeout);

    return HAL_TIMEOUT;
}

/* ============================================================================
 * SectorErase - 64KB Block Erase (수동 폴링 방식)
 *
 * 주의: MX25L12833F에서 "Sector"는 4KB이지만,
 *       STM32 External Loader의 SectorErase는 일반적으로 64KB 블록 단위
 * ============================================================================ */
__attribute__((used)) int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint32_t SectorAddr;
    uint8_t status_before = 0;
    uint8_t status_after = 0;
    
    g_last_erase_status = 0;  /* 초기화 */

    if (EraseStartAddress >= 0x90000000)
    {
        EraseStartAddress -= 0x90000000;
    }
    if (EraseEndAddress >= 0x90000000)
    {
        EraseEndAddress -= 0x90000000;
    }
    
    /* Sector (4KB) 경계로 정렬 */
    EraseStartAddress = EraseStartAddress - (EraseStartAddress % MEMORY_SECTOR_SIZE);
    
    while (EraseStartAddress < EraseEndAddress)
    {
        SectorAddr = EraseStartAddress;
        
        /* ===== 1. Write Enable ===== */
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
            g_last_erase_status = 0x10;
            return 0;
        }

        /* WEL 비트 확인 */
        HAL_Delay(1);
        if (OSPI_ReadStatusReg(&status_before) != HAL_OK)
        {
            g_last_erase_status = 0x11;
            return 0;
        }

        if ((status_before & STATUS_REG_WEL_MASK) == 0)
        {
            g_last_erase_status = 0x12;
            return 0;
        }
        
        /* ===== 2. Sector Erase Command (0x20) ===== */
        /* 수정: 64KB Block Erase(0xD8) 대신 4KB Sector Erase(0x20) 사용 */
        sCommand.Instruction        = SECTOR_ERASE_4K_CMD;
        sCommand.Address            = SectorAddr;
        sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
        sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
        sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
        sCommand.DataMode           = HAL_OSPI_DATA_NONE;
        
        if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
            g_last_erase_status = 0x20;
            return 0;
        }

        /* ===== 3. 수동 폴링으로 완료 대기 ===== */
        /* 데이터시트 tSE(Typical 40ms, Max 200ms) 고려 */

        /* WIP=0 될 때까지 대기 (최대 400ms로 설정) */
        if (OSPI_ManualWaitReady(400) != HAL_OK)
        {
            g_last_erase_status = 0x30;  /* Timeout */
            return 0;
        }
        
        /* ===== 4. 완료 후 상태 확인 ===== */
        if (OSPI_ReadStatusReg(&status_after) != HAL_OK)
        {
            g_last_erase_status = 0x40;
            return 0;
        }

        if (status_after & STATUS_REG_WEL_MASK)
        {
            g_last_erase_status = 0x41;
        }

        /* E_FAIL 비트 확인 */
        OSPI_ReadSecurityReg(&g_security_reg);
        if (g_security_reg & SECURITY_REG_E_FAIL_MASK)
        {
            g_last_erase_status = 0x50;
            return 0;
        }
        
        /* 다음 섹터로 이동 (4KB 증가) */
        EraseStartAddress += MEMORY_SECTOR_SIZE;
    }
    
    return 1;
}

/* ============================================================================
 * MassErase - Chip Erase (0xC7) - 수동 폴링 방식
 * ============================================================================ */
__attribute__((used)) int MassErase(uint32_t Parallelism)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint8_t status = 0;
    (void)Parallelism;
    
    g_last_erase_status = 0;
    
    /* ===== 1. Write Enable ===== */
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
        g_last_erase_status = 0x10;
        return 0;
    }

    /* WEL 확인 */
    HAL_Delay(1);
    if (OSPI_ReadStatusReg(&status) != HAL_OK)
    {
        g_last_erase_status = 0x11;
        return 0;
    }

    if ((status & STATUS_REG_WEL_MASK) == 0)
    {
        g_last_erase_status = 0x12;
        return 0;
    }

    /* ===== 2. Chip Erase Command (0xC7) ===== */
    sCommand.Instruction = CHIP_ERASE_CMD;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;

    if (HAL_OSPI_Command(&hospi1_local, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        g_last_erase_status = 0x20;
        return 0;
    }

    /* ===== 3. 수동 폴링으로 완료 대기 (최대 120초) ===== */
    /* Chip Erase는 오래 걸림 - 수동 폴링 사용 */
    if (OSPI_ManualWaitReady(120000) != HAL_OK)
    {
        g_last_erase_status = 0x30;
        return 0;
    }
    
    /* E_FAIL 비트 확인 */
    OSPI_ReadSecurityReg(&g_security_reg);
    if (g_security_reg & SECURITY_REG_E_FAIL_MASK)
    {
        g_last_erase_status = 0x50;
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
    uint8_t flash_buf[256];
    uint32_t VerifiedData = 0;
    uint64_t checksum = 0;
    uint32_t chunk_size;
    uint32_t flash_addr;
    
    (void)missalignement;
    Size *= 4;
    
    flash_addr = MemoryAddr;
    if (flash_addr >= 0x90000000)
    {
        flash_addr -= 0x90000000;
    }

    while (Size > VerifiedData)
    {
        chunk_size = (Size - VerifiedData) > 256 ? 256 : (Size - VerifiedData);

        /* Flash에서 읽기 */
        Read(0x90000000 + flash_addr + VerifiedData, chunk_size, flash_buf);

        for (uint32_t i = 0; i < chunk_size; i++)
        {
            if (flash_buf[i] != *(uint8_t*)(RAMBufferAddr + VerifiedData + i))
            {
                return ((checksum << 32) + (MemoryAddr + VerifiedData + i));
            }
            checksum += flash_buf[i];
        }

        VerifiedData += chunk_size;
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
 * Loader_OCTOSPI1_Init - 보수적인 설정 (5MHz)
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

    /* OSPI 설정 */
    hospi1_local.Instance = OCTOSPI1;
    hospi1_local.Init.FifoThreshold = 4;
    hospi1_local.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
    hospi1_local.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
    hospi1_local.Init.DeviceSize = 24;
    hospi1_local.Init.ChipSelectHighTime = 8;
    hospi1_local.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
    hospi1_local.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
    hospi1_local.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
    hospi1_local.Init.ClockPrescaler = 8;  /* 5MHz */
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
 * OSPI_WriteEnable - Write Enable (WREN: 0x06)
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_WriteEnable(void)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_AutoPollingTypeDef sConfig = {0};

    /* Send Write Enable command */
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

    /* Configure automatic polling to wait for WEL=1 */
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
 * OSPI_AutoPollingMemReady - Wait for WIP=0
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

    /* Wait for WIP=0 (Write In Progress = 0) */
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
 * OSPI_ResetMemory - Software Reset (RSTEN + RST)
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

    /* Wait for reset to complete (tREADY2 = 25ms for BE64K/BE32KB operation) */
    HAL_Delay(30);

    return HAL_OK;
}
