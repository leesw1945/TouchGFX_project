/**
 ******************************************************************************
 * @file    Loader_Src.c
 * @author  MCD Application Team (Modified for MX25L12833F + STM32U5G9)
 * @brief   External loader for MX25L12833F SPI NOR memory on STM32U5G9ZJT6Q.
 *
 *          - MSI 오실레이터 사용 (HSE 대신)
 *          - SysTick 우회 (HAL_InitTick, HAL_GetTick 오버라이드)
 *          - Quad I/O: Read 0xEB (1-4-4), Write 0x38 (1-4-4)
 ******************************************************************************
 */

#include "Loader_Src.h"
#include <string.h>

/* ============================================================================
 * MX25L12833F Commands
 * ============================================================================ */
#define RESET_ENABLE_CMD_LOCAL               0x66
#define RESET_MEMORY_CMD_LOCAL               0x99
#define READ_ID_CMD_LOCAL                    0x9F
#define READ_CMD_LOCAL                       0x03
#define READ_STATUS_REG_CMD_LOCAL            0x05
#define WRITE_STATUS_REG_CMD_LOCAL           0x01
#define READ_SECURITY_REG_CMD_LOCAL          0x2B
#define WRITE_ENABLE_CMD_LOCAL               0x06
#define SECTOR_ERASE_4K_CMD_LOCAL            0x20    /* 4KB Sector Erase */
#define CHIP_ERASE_CMD_LOCAL                 0xC7
#define GANG_BLOCK_UNLOCK_CMD_LOCAL          0x98
#define EXIT_QPI_MODE_CMD                    0xF5    /* Exit QPI Mode */
#define QUAD_PAGE_PROG_CMD_LOCAL             0x38    /* 4PP: Quad Page Program (1-4-4) */
#define QUAD_IO_READ_CMD_LOCAL               0xEB    /* 4READ: Quad I/O Read (1-4-4) */

/* Status Register Bit Masks */
#define STATUS_WIP                           0x01
#define STATUS_WEL                           0x02
#define STATUS_QE                            0x40    /* Quad Enable bit (bit 6) */
#define STATUS_BP_MASK                       0x3C

/* Security Register Bit Masks */
#define SECURITY_E_FAIL                      0x40
#define SECURITY_WPSEL                       0x80

/* Memory Parameters */
#define SECTOR_SIZE                          0x1000      /* 4KB Sector */
#define PAGE_SIZE                            0x100       /* 256 bytes */

/* Expected ID */
#define MANUFACTURER_ID                      0xC2

/* Timeouts */
#define TIMEOUT_WRITE_REG                    100
#define TIMEOUT_PAGE_PROG                    10
#define TIMEOUT_SECTOR_ERASE                 300
#define TIMEOUT_CHIP_ERASE_LOCAL             120000

/* ============================================================================
 * OSPI Handle & Variables
 * ============================================================================ */
static OSPI_HandleTypeDef hospi1_local;
static uint8_t g_flash_id[3] = {0, 0, 0};
static uint8_t g_status_reg = 0;
static uint8_t g_security_reg = 0;

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
static HAL_StatusTypeDef OSPI_EnableQuadMode(void);
static HAL_StatusTypeDef OSPI_ManualWaitReady(uint32_t Timeout);

/* ============================================================================
 * HAL Tick Override - Software Delay for External Loader
 * ============================================================================ */
KeepInCompilation HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    (void)TickPriority;
    return HAL_OK;
}

uint32_t HAL_GetTick(void)
{
    return 1;
}

void HAL_Delay(uint32_t Delay)
{
    volatile uint32_t i;
    for (i = 0; i < (Delay * 32000); i++)
    {
        __NOP();
    }
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

    if (OSPI_ReadStatusReg(&status_reg) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if ((status_reg & STATUS_BP_MASK) == 0)
    {
        return HAL_OK;
    }

    uint8_t new_status = status_reg & ~STATUS_BP_MASK;

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
 * OSPI_EnableQuadMode - QE 비트 활성화
 * ============================================================================ */
static HAL_StatusTypeDef OSPI_EnableQuadMode(void)
{
    uint8_t status_reg = 0;

    if (OSPI_ReadStatusReg(&status_reg) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (status_reg & STATUS_QE)
    {
        return HAL_OK;
    }

    status_reg |= STATUS_QE;

    if (OSPI_WriteStatusReg(status_reg) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (OSPI_ReadStatusReg(&status_reg) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if ((status_reg & STATUS_QE) == 0)
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
    uint32_t max_count = Timeout * 1000;

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

/* ============================================================================
 * Loader_SystemClock_Config - MSI + PLL → 160MHz
 * ============================================================================ */
static void Loader_SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    __HAL_RCC_PWR_CLK_DISABLE();

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;  /* 4MHz */
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 80;   /* 4MHz × 80 = 320MHz */
    RCC_OscInitStruct.PLL.PLLR = 2;    /* 320MHz / 2 = 160MHz */
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        while(1);
    }

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
        while(1);
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

    /* PA2=NCS, PA3=CLK, PA6=IO3, PA7=IO2 */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PB0=IO1, PB1=IO0 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hospi1_local.Instance = OCTOSPI1;
    hospi1_local.Init.FifoThreshold = 4;
    hospi1_local.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
    hospi1_local.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
    hospi1_local.Init.DeviceSize = 24;  /* 2^24 = 16MB */
    hospi1_local.Init.ChipSelectHighTime = 2;
    hospi1_local.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
    hospi1_local.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
    hospi1_local.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
    hospi1_local.Init.ClockPrescaler = 4;  /* 160MHz / 4 = 40MHz */
    hospi1_local.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
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
 * Init
 * ============================================================================ */
KeepInCompilation int Init(void)
{
    /* BSS 섹션 초기화 */
#if defined(__ICCARM__)
    char *startadd = __section_begin(".bss");
    uint32_t size = __section_size(".bss");
    memset(startadd, 0, size);
#else
    extern uint32_t _sbss, _ebss;
    uint32_t *pBss = &_sbss;
    while (pBss < &_ebss)
    {
        *pBss++ = 0;
    }
#endif

    memset(&hospi1_local, 0, sizeof(hospi1_local));

    SystemInit();
    HAL_Init();
    Loader_SystemClock_Config();
    Loader_OCTOSPI1_Init();

    HAL_Delay(100);

    /* QPI Reset (Flash가 QPI 모드라면 SPI 모드로 복귀) */
    OSPI_QPI_Reset();
    HAL_Delay(50);

    /* Memory Reset */
    if (OSPI_ResetMemory() != HAL_OK)
    {
        return 0;
    }

    HAL_Delay(100);

    /* Read ID */
    if (OSPI_ReadID(g_flash_id) != HAL_OK)
    {
        return 0;
    }

    if (g_flash_id[0] != MANUFACTURER_ID)
    {
        return 0;
    }

    /* Read Status & Security Register */
    OSPI_ReadStatusReg(&g_status_reg);
    OSPI_ReadSecurityReg(&g_security_reg);

    /* Block Protection 해제 */
    if (g_security_reg & SECURITY_WPSEL)
    {
        OSPI_GangBlockUnlock();
    }
    else
    {
        OSPI_ClearBlockProtection();
    }

    /* Quad Mode 활성화 */
    if (OSPI_EnableQuadMode() != HAL_OK)
    {
        return 0;
    }

    return 1;
}

/* ============================================================================
 * Read - 4READ (0xEB, 1-4-4 모드)
 * ============================================================================ */
KeepInCompilation int Read(uint32_t Address, uint32_t Size, uint8_t* buffer)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    if (Address >= 0x90000000)
    {
        Address -= 0x90000000;
    }

    sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction        = QUAD_IO_READ_CMD_LOCAL;  /* 0xEB */
    sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address            = Address;
    sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
    sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
    sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData             = Size;
    sCommand.DummyCycles        = 6;  /* MX25L12833F: 2 mode + 4 dummy */
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
 * Write - 4PP (0x38, 1-4-4 모드)
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

    /* QE 비트 확인 */
    {
        uint8_t qe_check = 0;
        OSPI_ReadStatusReg(&qe_check);
        if ((qe_check & STATUS_QE) == 0)
        {
            if (OSPI_EnableQuadMode() != HAL_OK)
            {
                return 0;
            }
        }
    }

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

        /* 4PP (0x38) - 1-4-4 모드 */
        sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
        sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
        sCommand.Instruction        = QUAD_PAGE_PROG_CMD_LOCAL;  /* 0x38 */
        sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
        sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
        sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
        sCommand.Address            = current_addr;
        sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
        sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
        sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
        sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
        sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
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
 * SectorErase - 4KB Sector Erase (0x20)
 * ============================================================================ */
KeepInCompilation int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

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
        if (OSPI_WriteEnable() != HAL_OK)
        {
            return 0;
        }

        /* 4KB Sector Erase (0x20) */
        memset(&sCommand, 0, sizeof(OSPI_RegularCmdTypeDef));

        sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
        sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
        sCommand.Instruction        = SECTOR_ERASE_4K_CMD_LOCAL;
        sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
        sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
        sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
        sCommand.Address            = EraseStartAddress;
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

        if (OSPI_ManualWaitReady(TIMEOUT_SECTOR_ERASE) != HAL_OK)
        {
            return 0;
        }

        /* E_FAIL 체크 */
        OSPI_ReadSecurityReg(&g_security_reg);
        if (g_security_reg & SECURITY_E_FAIL)
        {
            return 0;
        }

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
    (void)Parallelism;

    if (OSPI_WriteEnable() != HAL_OK)
    {
        return 0;
    }

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
        return 0;
    }

    HAL_Delay(1000);

    if (OSPI_ManualWaitReady(TIMEOUT_CHIP_ERASE_LOCAL) != HAL_OK)
    {
        return 0;
    }

    OSPI_ReadSecurityReg(&g_security_reg);
    if (g_security_reg & SECURITY_E_FAIL)
    {
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
