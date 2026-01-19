/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @author  Fixed Version v17 - 4KB Sector Erase ì‚¬ìš©
  * @brief   MX25L12833F External Loader (SPI 1-Line Mode)
  *
  * â˜…â˜…â˜… ìˆ˜ì • ì‚¬í•­ (v17) â˜…â˜…â˜…
  * - SectorErase: 4KB Sector Erase (0x20) ëª…ë ¹ ì‚¬ìš©
  * - Dev_Inf.cì™€ ì¼ì¹˜í•˜ë„ë¡ MEMORY_SECTOR_SIZE = 4KB ì‚¬ìš©
  * - ë””ë²„ê·¸ ë³€ìˆ˜ ìœ ì§€
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
#define READ_CMD                             0x03
#define PAGE_PROG_CMD                        0x02
#define READ_STATUS_REG_CMD                  0x05
#define WRITE_STATUS_REG_CMD                 0x01
#define READ_CONFIG_REG_CMD                  0x15
#define READ_SECURITY_REG_CMD                0x2B
#define WRITE_ENABLE_CMD                     0x06
#define WRITE_DISABLE_CMD                    0x04
#define SECTOR_ERASE_4K_CMD                  0x20    /* â˜… 4KB Sector Erase â˜… */
#define BLOCK_ERASE_32K_CMD                  0x52    /* 32KB Block Erase */
#define BLOCK_ERASE_64K_CMD                  0xD8    /* 64KB Block Erase */
#define CHIP_ERASE_CMD                       0xC7
#define GANG_BLOCK_UNLOCK_CMD                0x98

/* Status Register Bit Masks */
#define STATUS_REG_WIP_MASK                  0x01
#define STATUS_REG_WEL_MASK                  0x02
#define STATUS_REG_BP_MASK                   0x3C

/* Security Register Bit Masks */
#define SECURITY_REG_P_FAIL_MASK             0x20
#define SECURITY_REG_E_FAIL_MASK             0x40
#define SECURITY_REG_WPSEL_MASK              0x80

/* Memory Parameters - â˜… 4KB Sector ë‹¨ìœ„ â˜… */
#define MEMORY_FLASH_SIZE                    0x01000000  /* 16MB */
#define MEMORY_SECTOR_SIZE                   0x1000      /* â˜… 4KB Sector â˜… */
#define MEMORY_BLOCK_SIZE                    0x10000     /* 64KB Block (ì°¸ê³ ìš©) */
#define MEMORY_PAGE_SIZE                     0x100       /* 256 bytes */

/* Expected ID */
#define MX25L12833F_MANUFACTURER_ID          0xC2
#define MX25L12833F_MEMORY_TYPE              0x20
#define MX25L12833F_MEMORY_DENSITY           0x18

/* Timeouts */
#define TIMEOUT_WRITE_STATUS_REG             100
#define TIMEOUT_PAGE_PROGRAM                 10
#define TIMEOUT_SECTOR_ERASE_4K              300     /* â˜… 4KB Sector: max 120ms, ì—¬ìœ ìžˆê²Œ 300ms â˜… */
#define TIMEOUT_BLOCK_ERASE_64K              2000
#define TIMEOUT_CHIP_ERASE                   120000

/* ============================================================================
 * OSPI Handle & ì§„ë‹¨ ë³€ìˆ˜
 * ============================================================================ */
static OSPI_HandleTypeDef hospi1_local;
static uint8_t g_flash_id[3] = {0, 0, 0};
static uint8_t g_init_error_code = 0;
static uint8_t g_status_reg = 0;
static uint8_t g_security_reg = 0;
static uint8_t g_last_erase_status = 0;
static uint8_t g_debug_status1 = 0;
static uint8_t g_debug_status2 = 0;

/* v17 ë””ë²„ê·¸ ë³€ìˆ˜ */
static uint32_t g_erase_input_start = 0;
static uint32_t g_erase_input_end = 0;
static uint32_t g_erase_actual_addr = 0;
static uint8_t g_erase_call_count = 0;
static uint8_t g_erase_loop_count = 0;
static uint8_t g_data_before_erase[4] = {0};
static uint8_t g_data_after_erase[4] = {0};
static uint8_t g_loader_version = 0x20;  /* â˜… v17 ë§ˆì»¤ â˜… */

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
static HAL_StatusTypeDef OSPI_ManualWaitReady(uint32_t Timeout);
static HAL_StatusTypeDef OSPI_ReadBytes(uint32_t Address, uint8_t* buffer, uint32_t size);

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
 * OSPI_ReadBytes - ë‚´ë¶€ Read í•¨ìˆ˜
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_ReadBytes(uint32_t Address, uint8_t* buffer, uint32_t size)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

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

    if (OSPI_AutoPollingMemReady(TIMEOUT_WRITE_STATUS_REG) != HAL_OK)
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

    if ((status_reg & STATUS_REG_BP_MASK) == 0)
    {
        return HAL_OK;
    }

    new_status = status_reg & ~STATUS_REG_BP_MASK;

    if (OSPI_WriteStatusReg(new_status) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (OSPI_ReadStatusReg(&status_reg) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if ((status_reg & STATUS_REG_BP_MASK) != 0)
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

    if (OSPI_AutoPollingMemReady(TIMEOUT_WRITE_STATUS_REG) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* ============================================================================
 * Init
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
    g_debug_status1 = 0;
    g_debug_status2 = 0;

    // v20: 제거 -     g_erase_input_start = 0;
    // v20: 제거 -     g_erase_input_end = 0;
    // v20: 제거 -     g_erase_actual_addr = 0;
    // v20: 제거 -     g_erase_call_count = 0;
    // v20: 제거 -     g_erase_loop_count = 0;
    // v20: 제거 -     memset(g_data_before_erase, 0, 4);
    // v20: 제거 -     memset(g_data_after_erase, 0, 4);
    g_loader_version = 0x20;

    memset(&hospi1_local, 0, sizeof(hospi1_local));
    uwTick_local = 0;

    HAL_Init();
    Loader_SystemClock_Config();
    Loader_OCTOSPI1_Init();

    HAL_Delay(100);

    if (OSPI_ResetMemory() != HAL_OK)
    {
        g_init_error_code = 1;
        return 0;
    }

    HAL_Delay(100);

    if (OSPI_ReadID(g_flash_id) != HAL_OK)
    {
        g_init_error_code = 2;
        return 0;
    }

    if (OSPI_ReadStatusReg(&g_status_reg) != HAL_OK)
    {
        g_init_error_code = 3;
        return 0;
    }

    if (OSPI_ReadSecurityReg(&g_security_reg) != HAL_OK)
    {
        g_init_error_code = 6;
    }

    if (g_flash_id[0] != MX25L12833F_MANUFACTURER_ID)
    {
        g_init_error_code = 4;
    }

    if (g_security_reg & SECURITY_REG_WPSEL_MASK)
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

    OSPI_ReadStatusReg(&g_status_reg);
    OSPI_ReadSecurityReg(&g_security_reg);

    return 1;
}

/* ============================================================================
 * Read
 * ============================================================================ */
__attribute__((used)) int Read(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

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
            buffer[6] = (g_status_reg & STATUS_REG_BP_MASK) >> 2;
            buffer[7] = g_last_erase_status;
            buffer[8] = 0xAA;
            buffer[9] = 0x55;
            buffer[10] = (g_security_reg & SECURITY_REG_WPSEL_MASK) ? 1 : 0;
            buffer[11] = (g_security_reg & SECURITY_REG_E_FAIL_MASK) ? 1 : 0;
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
            buffer[6] = (g_status_reg & STATUS_REG_BP_MASK) >> 2;
            buffer[7] = g_last_erase_status;
            buffer[8] = 0xAA;
            buffer[9] = 0x55;
            buffer[10] = (g_security_reg & SECURITY_REG_WPSEL_MASK) ? 1 : 0;
            buffer[11] = (g_security_reg & SECURITY_REG_E_FAIL_MASK) ? 1 : 0;
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

    if (Address >= 0x90000000)
    {
        Address -= 0x90000000;
    }

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
 * Write
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
 * OSPI_ManualWaitReady
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

        if ((status & STATUS_REG_WIP_MASK) == 0)
        {
            return HAL_OK;
        }

        HAL_Delay(1);

    } while ((HAL_GetTick() - tickstart) < Timeout);

    return HAL_TIMEOUT;
}

/* ============================================================================
 * SectorErase - â˜…â˜…â˜… 4KB Sector Erase (0x20) â˜…â˜…â˜…
 *
 * Dev_Inf.cì™€ ì¼ì¹˜: 4096 Sectors Ã— 4KB = 16MB
 * ============================================================================ */
__attribute__((used)) int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    OSPI_RegularCmdTypeDef sCommand;
    uint32_t SectorAddr;
    uint8_t status_before = 0;
    uint8_t status_after = 0;

    /* ë””ë²„ê·¸: ìž…ë ¥ ì£¼ì†Œ ì €ìž¥ */
    g_erase_input_start = EraseStartAddress;
    g_erase_input_end = EraseEndAddress;
    g_erase_call_count++;
    g_erase_loop_count = 0;

    g_last_erase_status = 0;
    g_debug_status1 = 0;
    g_debug_status2 = 0;

    /* ì£¼ì†Œ ë³€í™˜ */
    if (EraseStartAddress >= 0x90000000)
    {
        EraseStartAddress -= 0x90000000;
    }
    if (EraseEndAddress >= 0x90000000)
    {
        EraseEndAddress -= 0x90000000;
    }

    /* â˜… 4KB Sector ê²½ê³„ë¡œ ì •ë ¬ â˜… */
    EraseStartAddress = EraseStartAddress - (EraseStartAddress % MEMORY_SECTOR_SIZE);
    

    /* v20: Start == End fix - ensure at least one sector is erased */
    if (EraseEndAddress <= EraseStartAddress)
    {
        EraseEndAddress = EraseStartAddress + MEMORY_SECTOR_SIZE;
    }
    while (EraseStartAddress < EraseEndAddress)
    {
        g_erase_loop_count++;
        SectorAddr = EraseStartAddress;
        g_erase_actual_addr = SectorAddr;

        /* Erase ì „ ë°ì´í„° ì½ê¸° */
        OSPI_ReadBytes(SectorAddr, g_data_before_erase, 4);

        /* 1. Write Enable */
        memset(&sCommand, 0, sizeof(OSPI_RegularCmdTypeDef));

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

        HAL_Delay(1);

        if (OSPI_ReadStatusReg(&status_before) != HAL_OK)
        {
            g_last_erase_status = 0x11;
            return 0;
        }

        g_debug_status2 = status_before;

        if ((status_before & STATUS_REG_WEL_MASK) == 0)
        {
            g_last_erase_status = 0x12;
            return 0;
        }

        /* â˜…â˜…â˜… 2. 4KB Sector Erase Command (0x20) â˜…â˜…â˜… */
        memset(&sCommand, 0, sizeof(OSPI_RegularCmdTypeDef));

        sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
        sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
        sCommand.Instruction        = SECTOR_ERASE_4K_CMD;  /* â˜… 0x20 â˜… */
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

        /* 3. Erase ëª…ë ¹ ì§í›„ ìƒíƒœ ì €ìž¥ */
        HAL_Delay(1);
        OSPI_ReadStatusReg(&g_debug_status1);

        /* 4. Erase ì™„ë£Œ ëŒ€ê¸° (4KBëŠ” ìµœëŒ€ 120ms) */
        HAL_Delay(10);

        if (OSPI_ManualWaitReady(TIMEOUT_SECTOR_ERASE_4K) != HAL_OK)
        {
            g_last_erase_status = 0x30;
            return 0;
        }
        
        /* 5. ì™„ë£Œ í›„ ìƒíƒœ í™•ì¸ */
        if (OSPI_ReadStatusReg(&status_after) != HAL_OK)
        {
            g_last_erase_status = 0x40;
            return 0;
        }

        if (status_after & STATUS_REG_WEL_MASK)
        {
            g_last_erase_status = 0x41;
        }

        OSPI_ReadSecurityReg(&g_security_reg);
        if (g_security_reg & SECURITY_REG_E_FAIL_MASK)
        {
            g_last_erase_status = 0x50;
            return 0;
        }

        /* Erase í›„ ë°ì´í„° ì½ê¸° */
        OSPI_ReadBytes(SectorAddr, g_data_after_erase, 4);

        /* â˜… 4KB ë‹¨ìœ„ë¡œ ì¦ê°€ â˜… */
        EraseStartAddress += MEMORY_SECTOR_SIZE;
    }
    
    return 1;
}

/* ============================================================================
 * MassErase
 * ============================================================================ */
__attribute__((used)) int MassErase(uint32_t Parallelism)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint8_t status = 0;
    (void)Parallelism;

    g_last_erase_status = 0;

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

    memset(&sCommand, 0, sizeof(OSPI_RegularCmdTypeDef));

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
        g_last_erase_status = 0x20;
        return 0;
    }

    HAL_Delay(1000);

    if (OSPI_ManualWaitReady(TIMEOUT_CHIP_ERASE) != HAL_OK)
    {
        g_last_erase_status = 0x30;
        return 0;
    }

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
        sCommand.Instruction        = READ_CMD;
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
 * ============================================================================ */
static void Loader_SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

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
    RCC_OscInitStruct.PLL.PLLN = 40;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
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
}

/* ============================================================================
 * Loader_OCTOSPI1_Init
 * ============================================================================ */
static void Loader_OCTOSPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    OSPIM_CfgTypeDef sOspiManagerCfg = {0};

    __HAL_RCC_OSPIM_CLK_ENABLE();
    __HAL_RCC_OSPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hospi1_local.Instance = OCTOSPI1;
    hospi1_local.Init.FifoThreshold = 4;
    hospi1_local.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
    hospi1_local.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
    hospi1_local.Init.DeviceSize = 24;
    hospi1_local.Init.ChipSelectHighTime = 8;
    hospi1_local.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
    hospi1_local.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
    hospi1_local.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
    hospi1_local.Init.ClockPrescaler = 8;
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

    HAL_Delay(30);

    return HAL_OK;
}
