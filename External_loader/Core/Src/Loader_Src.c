/**
 ******************************************************************************
 * @file    Loader_Src.c
 * @author  Fixed Version v17 - 4KB Sector Erase 사용
 * @brief   MX25L12833F External Loader (SPI 1-Line Mode)
 *
 * ★★★ 수정 사항 (v17) ★★★
 * - SectorErase: 4KB Sector Erase (0x20) 명령 사용
 * - Dev_Inf.c와 일치하도록 MEMORY_SECTOR_SIZE = 4KB 사용
 * - 디버그 변수 유지
 *
 * ★★★ 디버깅 모드 ★★★
 * - DEBUG_MODE 정의 시: HAL 오버라이드 비활성화 (STM32CubeIDE 디버깅용)
 * - 미정의 시: External Loader 모드 (HAL 오버라이드 활성화)
 *
 * ★★★ STM32CubeIDE에서 디버깅하려면 아래 DEBUG_MODE 정의를 활성화하세요 ★★★
 ******************************************************************************
 */

/* ========================================================================
 * ★★★ IDE 디버깅 모드 ★★★
 * STM32CubeIDE에서 이 파일을 디버깅할 때는 아래 주석을 해제하세요.
 * 실제 External Loader(.stldr) 빌드 시에는 다시 주석 처리하세요.
 *
 * 또는 프로젝트 설정에서 전처리기 매크로로 DEBUG_MODE를 추가하세요:
 * Properties → C/C++ Build → Settings → Preprocessor → Define symbols
 * ======================================================================== */
#define DEBUG_MODE

#include "stm32u5xx_hal.h"
#include <string.h>

/* DEBUG_MODE가 정의되지 않으면 External Loader 모드 */
#ifndef DEBUG_MODE
#define EXTERNAL_LOADER_MODE
#endif

/* ============================================================================
 * MX25L12833F Commands
 * ============================================================================
 */
#define RESET_ENABLE_CMD 0x66
#define RESET_MEMORY_CMD 0x99
#define READ_ID_CMD 0x9F
#define READ_CMD 0x03
#define PAGE_PROG_CMD 0x02
#define READ_STATUS_REG_CMD 0x05
#define WRITE_STATUS_REG_CMD 0x01
#define READ_CONFIG_REG_CMD 0x15
#define READ_SECURITY_REG_CMD 0x2B
#define WRITE_ENABLE_CMD 0x06
#define WRITE_DISABLE_CMD 0x04
#define SECTOR_ERASE_4K_CMD 0x20 /* ★ 4KB Sector Erase ★ */
#define BLOCK_ERASE_32K_CMD 0x52 /* 32KB Block Erase */
#define BLOCK_ERASE_64K_CMD 0xD8 /* 64KB Block Erase */
#define CHIP_ERASE_CMD 0xC7
#define GANG_BLOCK_UNLOCK_CMD 0x98

/* Status Register Bit Masks */
#define STATUS_REG_WIP_MASK 0x01
#define STATUS_REG_WEL_MASK 0x02
#define STATUS_REG_BP_MASK 0x3C

/* Security Register Bit Masks */
#define SECURITY_REG_P_FAIL_MASK 0x20
#define SECURITY_REG_E_FAIL_MASK 0x40
#define SECURITY_REG_WPSEL_MASK 0x80

/* Memory Parameters - ★ 4KB Sector 단위 ★ */
#define MEMORY_FLASH_SIZE 0x01000000 /* 16MB */
#define MEMORY_SECTOR_SIZE 0x1000    /* ★ 4KB Sector ★ */
#define MEMORY_BLOCK_SIZE 0x10000    /* 64KB Block (참고용) */
#define MEMORY_PAGE_SIZE 0x100       /* 256 bytes */

/* Expected ID */
#define MX25L12833F_MANUFACTURER_ID 0xC2
#define MX25L12833F_MEMORY_TYPE 0x20
#define MX25L12833F_MEMORY_DENSITY 0x18

/* Timeouts */
#define TIMEOUT_WRITE_STATUS_REG 100
#define TIMEOUT_PAGE_PROGRAM 10
#define TIMEOUT_SECTOR_ERASE_4K 1000 // 4KB Sector: 기존 max 120ms에서  여유있게 1000ms로 시간을 좀 늘렸다.
#define TIMEOUT_BLOCK_ERASE_64K 2000
#define TIMEOUT_CHIP_ERASE 120000

/* ============================================================================
 * OSPI Handle & 진단 변수
 * ============================================================================
 */
static OSPI_HandleTypeDef hospi1_local;
static uint8_t g_flash_id[3] = {0, 0, 0};
static uint8_t g_init_error_code = 0;
static uint8_t g_status_reg = 0;
static uint8_t g_security_reg = 0;
static uint8_t g_last_erase_status = 0;
static uint8_t g_debug_status1 = 0;
static uint8_t g_debug_status2 = 0;

/* v17 디버그 변수 */
static uint32_t g_erase_input_start = 0;
static uint32_t g_erase_input_end = 0;
static uint32_t g_erase_actual_addr = 0;
static uint8_t g_erase_call_count = 0;
static uint8_t g_erase_loop_count = 0;
static uint8_t g_data_before_erase[4] = {0};
static uint8_t g_data_after_erase[4] = {0};
static uint8_t g_loader_version = 0x17; /* v17 마커  */

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================
 */
static void Loader_SystemClock_Config(void);
static void Loader_OCTOSPI1_Init(void);
static HAL_StatusTypeDef OSPI_WriteEnable(void);
static HAL_StatusTypeDef OSPI_AutoPollingMemReady(uint32_t Timeout);
static HAL_StatusTypeDef OSPI_ResetMemory(void);
static HAL_StatusTypeDef OSPI_ReadID(uint8_t *id);
static HAL_StatusTypeDef OSPI_ReadStatusReg(uint8_t *status);
static HAL_StatusTypeDef OSPI_WriteStatusReg(uint8_t status);
static HAL_StatusTypeDef OSPI_ReadSecurityReg(uint8_t *security);
static HAL_StatusTypeDef OSPI_ClearBlockProtection(void);
static HAL_StatusTypeDef OSPI_GangBlockUnlock(void);
static HAL_StatusTypeDef OSPI_ManualWaitReady(uint32_t Timeout);
static HAL_StatusTypeDef OSPI_ManualWaitReady(uint32_t Timeout);
static HAL_StatusTypeDef OSPI_ReadBytes(uint32_t Address, uint8_t *buffer,
                                        uint32_t size);
static HAL_StatusTypeDef OSPI_QPI_Reset(void); /* QPI Reset Prototype */

/* ============================================================================
 * HAL Tick Override - External Loader 전용
 * DEBUG_MODE 정의 시 비활성화 (일반 HAL 사용)
 * ============================================================================
 */
#ifdef EXTERNAL_LOADER_MODE
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
  (void)TickPriority;
  return HAL_OK;
}

uint32_t HAL_GetTick(void) { return 0; }

// HAL_Delay() SysTick을 사용하는데 External Loader에서는
// SysTick(인터럽트)을 사용하지 않기 때문에 대체한다.
void HAL_Delay(uint32_t Delay) {
  volatile uint32_t i;

  for (i = 0; i < (Delay * 200000); i++) {
    __NOP();
  }
}

void HAL_IncTick(void) { /* Not used */ }
#endif /* EXTERNAL_LOADER_MODE */

/* ============================================================================
 * OSPI_ReadBytes - 내부 Read 함수
 * ============================================================================
 */
static HAL_StatusTypeDef OSPI_ReadBytes(uint32_t Address, uint8_t *buffer,
                                        uint32_t size) {
  OSPI_RegularCmdTypeDef sCommand = {0};

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = READ_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Address = Address;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
  sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData = size;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Receive(&hospi1_local, buffer, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* ============================================================================
 * OSPI_ReadID
 * ============================================================================
 */
static HAL_StatusTypeDef OSPI_ReadID(uint8_t *id) {
  OSPI_RegularCmdTypeDef sCommand = {0};

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = READ_ID_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData = 3;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Receive(&hospi1_local, id, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* ============================================================================
 * OSPI_ReadStatusReg
 * ============================================================================
 */
static HAL_StatusTypeDef OSPI_ReadStatusReg(uint8_t *status) {
  OSPI_RegularCmdTypeDef sCommand = {0};

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = READ_STATUS_REG_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData = 1;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Receive(&hospi1_local, status, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* ============================================================================
 * OSPI_WriteStatusReg
 * ============================================================================
 */
static HAL_StatusTypeDef OSPI_WriteStatusReg(uint8_t status) {
  OSPI_RegularCmdTypeDef sCommand = {0};

  if (OSPI_WriteEnable() != HAL_OK) {
    return HAL_ERROR;
  }

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = WRITE_STATUS_REG_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData = 1;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Transmit(&hospi1_local, &status,
                        HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  if (OSPI_AutoPollingMemReady(TIMEOUT_WRITE_STATUS_REG) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* ============================================================================
 * OSPI_ReadSecurityReg
 * ============================================================================
 */
static HAL_StatusTypeDef OSPI_ReadSecurityReg(uint8_t *security) {
  OSPI_RegularCmdTypeDef sCommand = {0};

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = READ_SECURITY_REG_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData = 1;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_OSPI_Receive(&hospi1_local, security,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* ============================================================================
 * OSPI_ClearBlockProtection
 * ============================================================================
 */
static HAL_StatusTypeDef OSPI_ClearBlockProtection(void) {
  uint8_t status_reg = 0;
  uint8_t new_status = 0;

  if (OSPI_ReadStatusReg(&status_reg) != HAL_OK) {
    return HAL_ERROR;
  }

  if ((status_reg & STATUS_REG_BP_MASK) == 0) {
    return HAL_OK;
  }

  new_status = status_reg & ~STATUS_REG_BP_MASK;

  if (OSPI_WriteStatusReg(new_status) != HAL_OK) {
    return HAL_ERROR;
  }

  if (OSPI_ReadStatusReg(&status_reg) != HAL_OK) {
    return HAL_ERROR;
  }

  if ((status_reg & STATUS_REG_BP_MASK) != 0) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* ============================================================================
 * OSPI_GangBlockUnlock
 * ============================================================================
 */
static HAL_StatusTypeDef OSPI_GangBlockUnlock(void) {
  OSPI_RegularCmdTypeDef sCommand = {0};

  if (OSPI_WriteEnable() != HAL_OK) {
    return HAL_ERROR;
  }

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = GANG_BLOCK_UNLOCK_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  if (OSPI_AutoPollingMemReady(TIMEOUT_WRITE_STATUS_REG) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* ============================================================================
 * Init
 * ============================================================================
 */
__attribute__((used)) int Init(void) {
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
  g_loader_version = 0x17;

  memset(&hospi1_local, 0, sizeof(hospi1_local));
  //uwTick_local = 0;

//  if (HAL_OSPI_Init(&hospi1_local) != HAL_OK) {
//    return 0;
//  }

  Loader_OCTOSPI1_Init();

  HAL_Delay(100);

  /* QPI Reset 시도 (만약 Flash가 QPI 모드라면 SPI 모드로 복귀) */
  /* 타임아웃 방지를 위해 무조건 시도 */
  OSPI_QPI_Reset();

  HAL_Delay(50);

  if (OSPI_ResetMemory() != HAL_OK) {
    g_init_error_code = 1;
    return 0;
  }

  HAL_Delay(100);

  if (OSPI_ReadID(g_flash_id) != HAL_OK) {
    g_init_error_code = 2;
    return 0; /* Fail-Fast: ID 읽기 실패 시 즉시 리턴 */
  }

  /* ID 검증 */
  if (g_flash_id[0] != MX25L12833F_MANUFACTURER_ID) {
    g_init_error_code = 4;
    return 0; /* Fail-Fast: ID 불일치 시 즉시 리턴 */
  }

  if (OSPI_ReadStatusReg(&g_status_reg) != HAL_OK) {
    g_init_error_code = 3;
    return 0;
  }

  if (OSPI_ReadSecurityReg(&g_security_reg) != HAL_OK) {
    g_init_error_code = 6;
  }

  /* Block Protection 해제 */
  if (g_security_reg & SECURITY_REG_WPSEL_MASK) {
    if (OSPI_GangBlockUnlock() != HAL_OK) {
      g_init_error_code = 7;
    }
  } else {
    if (OSPI_ClearBlockProtection() != HAL_OK) {
      g_init_error_code = 5;
    }
  }

  OSPI_ReadStatusReg(&g_status_reg);
  OSPI_ReadSecurityReg(&g_security_reg);

  return 1;
}

/* ============================================================================
 * Read
 * ============================================================================
 */
__attribute__((used)) int Read(uint32_t Address, uint32_t Size,
                               uint8_t *buffer) {
  OSPI_RegularCmdTypeDef sCommand = {0};

  if (Address == 0x90000090) {
    OSPI_ReadStatusReg(&g_status_reg);
    OSPI_ReadSecurityReg(&g_security_reg);

    if (Size >= 32) {
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
    } else if (Size >= 16) {
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

  if (Address == 0x90000080) {
    if (Size >= 4) {
      buffer[0] = g_loader_version;
      buffer[1] = g_data_before_erase[0];
      buffer[2] = g_data_before_erase[1];
      buffer[3] = g_data_before_erase[2];
    }
    return 1;
  }

  if (Address >= 0x90000000) {
    Address -= 0x90000000;
  }

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = READ_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.Address = Address;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
  sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
  sCommand.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData = Size;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return 0;
  }

  if (HAL_OSPI_Receive(&hospi1_local, buffer, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return 0;
  }

  return 1;
}

/* ============================================================================
 * Write
 * ============================================================================
 */
__attribute__((used)) int Write(uint32_t Address, uint32_t Size,
                                uint8_t *buffer) {
  OSPI_RegularCmdTypeDef sCommand = {0};
  uint32_t end_addr, current_size, current_addr;

  if (Address >= 0x90000000) {
    Address -= 0x90000000;
  }

  current_addr = Address;
  end_addr = Address + Size;

  while (current_addr < end_addr) {
    current_size = MEMORY_PAGE_SIZE - (current_addr % MEMORY_PAGE_SIZE);
    if (current_size > (end_addr - current_addr)) {
      current_size = end_addr - current_addr;
    }

    if (OSPI_WriteEnable() != HAL_OK) {
      return 0;
    }

    sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction = PAGE_PROG_CMD;
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address = current_addr;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
    sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData = current_size;
    sCommand.DummyCycles = 0;
    sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                         HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      return 0;
    }

    if (HAL_OSPI_Transmit(&hospi1_local, buffer,
                          HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      return 0;
    }

    if (OSPI_AutoPollingMemReady(TIMEOUT_PAGE_PROGRAM) != HAL_OK) {
      return 0;
    }

    current_addr += current_size;
    buffer += current_size;
  }

  return 1;
}

/* ============================================================================
 * OSPI_ManualWaitReady
 * ============================================================================
 */
static HAL_StatusTypeDef OSPI_ManualWaitReady(uint32_t Timeout) {
  uint8_t status = 0;
  uint32_t loop_count = 0;

  do {
    if (OSPI_ReadStatusReg(&status) != HAL_OK) {
      return HAL_ERROR;
    }

    if ((status & STATUS_REG_WIP_MASK) == 0) {
      return HAL_OK;
    }

    HAL_Delay(1);
    loop_count++;

  } while (loop_count < Timeout);

  return HAL_TIMEOUT;
}

/* ============================================================================
 * SectorErase - ★★★ 4KB Sector Erase (0x20) ★★★
 *
 * Dev_Inf.c와 일치: 4096 Sectors × 4KB = 16MB
 * ============================================================================
 */
__attribute__((used)) int SectorErase(uint32_t EraseStartAddress,
                                      uint32_t EraseEndAddress) {
  OSPI_RegularCmdTypeDef
      sCommand;        // OSPI 명령 구조체(명령어, 주소, 데이터 모드 등 설정)
  uint32_t SectorAddr; // 현재 삭제할 섹터의 실제 주소
  uint8_t status_before = 0; // Write Enable 후 Status Resister 값
  uint8_t status_after = 0;  // Erase 완료 후 Status Resister 값

  /* 디버그: 입력 주소 저장 */ // sectoer erase 실행 후 0x90000090 Read하여 값을
                               // 확인하는 방식으로 디버딩
  g_erase_input_start = EraseStartAddress;
  g_erase_input_end = EraseEndAddress;
  g_erase_call_count++;   // SectorErase 함수 호출 횟수 누적
  g_erase_loop_count = 0; // while 루프 실행 횟수 (초기화)

  g_last_erase_status = 0; // 에러 코드 (0x10, 0x20 등)
  g_debug_status1 =
      0; // Write Enable 명령 직후 Status Register 값 (WEL 비트 확인용)
  g_debug_status2 = 0; // Erase 명령 직후 Status Register 값 (WIP 비트 확인용)

  /* 주소 변환 */
  if (EraseStartAddress >=
      0x90000000) // 0x90000000은 MCU의 external 메모리 주소
  {
    EraseStartAddress -= 0x90000000; // 0x90001000 - 0x90000000 == 0x00001000
                                     // (실제 플래시의 메모리 주소)
  }
  if (EraseEndAddress >= 0x90000000) {
    EraseEndAddress -= 0x90000000;
  }

  /* ★ 4KB Sector 경계로 정렬 ★ */ // 4KB 경계로 내림 정렬
  EraseStartAddress =
      EraseStartAddress - (EraseStartAddress % MEMORY_SECTOR_SIZE);

  EraseEndAddress = EraseEndAddress - (EraseEndAddress % MEMORY_SECTOR_SIZE);

  // 메인 Erase 루프
  while (EraseStartAddress <= EraseEndAddress) {
    g_erase_loop_count++;
    SectorAddr = EraseStartAddress;   // 현재 삭제할 섹터 주소
    g_erase_actual_addr = SectorAddr; // 실제로 Erase 명령이 전송된 주소를
                                      // 저장하는 디버그 변수 (ex 0x00001000)

    /* Erase 전 데이터 읽기 */ // Erase 전 첫 4바이트를 저장, 나중에 삭제 성공
                               // 여부 확인용
    OSPI_ReadBytes(SectorAddr, g_data_before_erase, 4);

    /* 1. Write Enable */ // 플래시 쓰기/삭제 전 반드시 Write Enable 필요
    // 이전 설정값이 남아있으면 오동작 가능하므로 항상 모든 구조체 멤버 0으로
    // 초기화
    memset(&sCommand, 0, sizeof(OSPI_RegularCmdTypeDef));

    sCommand.OperationType =
        HAL_OSPI_OPTYPE_COMMON_CFG; // 일반적인 명령 동작 (읽기/쓰기/삭제 등)
    sCommand.FlashId = HAL_OSPI_FLASH_ID_1; // OCTOSPI에 연결된 플래시 선택
    sCommand.Instruction =
        WRITE_ENABLE_CMD; // 전송할 SPI 명령어 / Write Enable 명령 = 0x06
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize =
        HAL_OSPI_INSTRUCTION_8_BITS; // 명령어 크기 = 8비트 (1바이트)
    sCommand.InstructionDtrMode =
        HAL_OSPI_INSTRUCTION_DTR_DISABLE; // DTR (Double Transfer Rate) 비활성화
    sCommand.AddressMode =
        HAL_OSPI_ADDRESS_NONE; // 주소 전송 안함 / Write Enable은 주소가 필요
                               // 없는 명령
    sCommand.AlternateBytesMode =
        HAL_OSPI_ALTERNATE_BYTES_NONE;       // Alternate Bytes 사용 안함
    sCommand.DataMode = HAL_OSPI_DATA_NONE;  // 데이터 전송 안함
    sCommand.DummyCycles = 0;                // Dummy 클럭 사이클 없음
    sCommand.DQSMode = HAL_OSPI_DQS_DISABLE; // DQS (Data Strobe) 비활성화
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD; // SIOO 비활성화

    if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                         HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      g_last_erase_status =
          0x10; // 명령 전송 실패 시 에러 코드 0x10 저장 후 종료
      return 0;
    }

    HAL_Delay(1);

    if (OSPI_ReadStatusReg(&status_before) != HAL_OK) {
      g_last_erase_status = 0x11; // Status Register 읽기 실패
      return 0;
    }

    g_debug_status2 = status_before;

    // Status Register의 WEL (Write Enable Latch) 비트 확인
    if ((status_before & STATUS_REG_WEL_MASK) ==
        0) // STATUS_REG_WEL_MASK = 0x02 (bit 1)
    {
      // WEL=1이어야 쓰기/삭제 가능
      g_last_erase_status = 0x12; // WEL=0이면 Write Enable 실패 → 에러 0x12
      return 0;
    }

    /* ★★★ 2. 4KB Sector Erase Command (0x20) ★★★ */
    // 다시 초기화 (Write Enable 설정 제거), 구조체는 MCU RAM에 있고 WEL 비트는
    // 플래시 칩 내부에 이미 1로 설정됨 Write Enable 설정을 위한 구조체를
    // 초기화하는 것이고 플래시 내부에는 이미 Write Enable 가능한 상태 이전
    // 설정값이 남아있으면 오동작 가능하므로 항상 모든 구조체 멤버 0으로 초기화
    memset(&sCommand, 0, sizeof(OSPI_RegularCmdTypeDef));

    sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction =
        SECTOR_ERASE_4K_CMD; // 0x20: MX25L12833F의 4KB Sector Erase 명령
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address = SectorAddr; // 삭제할 섹터 주소
    sCommand.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = HAL_OSPI_DATA_NONE;
    sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData = 0;
    sCommand.DummyCycles = 0;
    sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                         HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      g_last_erase_status = 0x20; // Erase 명령 전송 실패
      return 0;
    }

    /* 3. Erase 명령 직후 상태 저장 */
    // Erase 명령 직후 Status Register 저장 (디버깅용)
    // 이 시점에서 WIP (Write In Progress) 비트가 1이어야 정상
    // WIP=1이면 플래시가 내부적으로 삭제 작업 중
    HAL_Delay(1);
    OSPI_ReadStatusReg(&g_debug_status1);

    /* 4. Erase 완료 대기 (4KB는 최대 120ms) */
    HAL_Delay(10); // 최소 대기 시간 (플래시 내부 작업 시작)

    // Status Register의 WIP 비트가 0이 될 때까지 폴링
    // TIMEOUT_SECTOR_ERASE_4K = 300ms (데이터시트 최대 120ms + 여유)
    if (OSPI_ManualWaitReady(TIMEOUT_SECTOR_ERASE_4K) != HAL_OK) {
      g_last_erase_status = 0x30; // 타임아웃 발생 시 에러 0x30
      return 0;
    }

    /* 5. 완료 후 상태 확인 */
    if (OSPI_ReadStatusReg(&status_after) != HAL_OK) {
      g_last_erase_status = 0x40; // Status 읽기 실패
      return 0;
    }

    // Erase 완료 후 WEL 비트는 자동으로 0이 되어야 함
    if (status_after & STATUS_REG_WEL_MASK) {
      g_last_erase_status = 0x41; // WEL=1이면 비정상 (경고만, 계속 진행)
    }

    OSPI_ReadSecurityReg(&g_security_reg);
    if (g_security_reg &
        SECURITY_REG_E_FAIL_MASK) // SECURITY_REG_E_FAIL_MASK = 0x40 (bit 6)
    {
      g_last_erase_status = 0x50; // Erase 실패 플래그 감지
      return 0;
    }

    /* Erase 후 데이터 읽기 */
    // 삭제 후 첫 4바이트 읽기 → 0xFFFFFFFF이어야 정상
    OSPI_ReadBytes(SectorAddr, g_data_after_erase, 4);

    /* ★ 4KB 단위로 증가 ★ */
    // MEMORY_SECTOR_SIZE (0x1000 = 4KB) 만큼 주소 증가
    // 다음 섹터로 이동하여 루프 계속
    EraseStartAddress += MEMORY_SECTOR_SIZE;
  }
  // 모든 섹터 삭제 완료 시 1 반환 (성공)
  // 중간에 에러 발생 시 0 반환 (실패)
  return 1;
}

/* ============================================================================
 * MassErase
 * ============================================================================
 */
__attribute__((used)) int MassErase(uint32_t Parallelism) {
  OSPI_RegularCmdTypeDef sCommand = {0};
  uint8_t status = 0;
  (void)Parallelism;

  g_last_erase_status = 0;

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = WRITE_ENABLE_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    g_last_erase_status = 0x10;
    return 0;
  }

  HAL_Delay(1);
  if (OSPI_ReadStatusReg(&status) != HAL_OK) {
    g_last_erase_status = 0x11;
    return 0;
  }

  if ((status & STATUS_REG_WEL_MASK) == 0) {
    g_last_erase_status = 0x12;
    return 0;
  }

  memset(&sCommand, 0, sizeof(OSPI_RegularCmdTypeDef));

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = CHIP_ERASE_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    g_last_erase_status = 0x20;
    return 0;
  }

  HAL_Delay(1000);

  if (OSPI_ManualWaitReady(TIMEOUT_CHIP_ERASE) != HAL_OK) {
    g_last_erase_status = 0x30;
    return 0;
  }

  OSPI_ReadSecurityReg(&g_security_reg);
  if (g_security_reg & SECURITY_REG_E_FAIL_MASK) {
    g_last_erase_status = 0x50;
    return 0;
  }

  return 1;
}

/* ============================================================================
 * Verify
 * ============================================================================
 */
__attribute__((used)) uint64_t Verify(uint32_t MemoryAddr,
                                      uint32_t RAMBufferAddr, uint32_t Size,
                                      uint32_t missalignement) {
  uint32_t VerifiedData = 0;
  uint32_t InitialOffset = (missalignement & 0xF);
  uint8_t *pFlash = (uint8_t *)(MemoryAddr);
  uint8_t *pRAM = (uint8_t *)(RAMBufferAddr);
  uint8_t flashByte;
  OSPI_RegularCmdTypeDef sCommand = {0};
  uint32_t flashAddr;

  pRAM += InitialOffset;
  pFlash += InitialOffset;

  while (VerifiedData < (Size - InitialOffset)) {
    flashAddr = (uint32_t)pFlash;
    if (flashAddr >= 0x90000000) {
      flashAddr -= 0x90000000;
    }

    sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
    sCommand.Instruction = READ_CMD;
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    sCommand.Address = flashAddr;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
    sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    sCommand.NbData = 1;
    sCommand.DummyCycles = 0;
    sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                         HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      return (uint64_t)((uint32_t)pFlash);
    }

    if (HAL_OSPI_Receive(&hospi1_local, &flashByte,
                         HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      return (uint64_t)((uint32_t)pFlash);
    }

    if (flashByte != *pRAM) {
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
 */
static void Loader_SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
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

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    return;
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    return;
  }
}

/* ============================================================================
 * Loader_OCTOSPI1_Init
 * ============================================================================
 */
static void Loader_OCTOSPI1_Init(void) {
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

  if (HAL_OSPI_Init(&hospi1_local) != HAL_OK) {
    return;
  }

  sOspiManagerCfg.ClkPort = 1;
  sOspiManagerCfg.NCSPort = 1;
  sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;

  if (HAL_OSPIM_Config(&hospi1_local, &sOspiManagerCfg,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return;
  }
}

/* ============================================================================
 * OSPI_WriteEnable
 * ============================================================================
 */
static HAL_StatusTypeDef OSPI_WriteEnable(void) {
  OSPI_RegularCmdTypeDef sCommand = {0};
  OSPI_AutoPollingTypeDef sConfig = {0};

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = WRITE_ENABLE_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  sCommand.Instruction = READ_STATUS_REG_CMD;
  sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
  sCommand.NbData = 1;
  sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  sConfig.Match = STATUS_REG_WEL_MASK;
  sConfig.Mask = STATUS_REG_WEL_MASK;
  sConfig.MatchMode = HAL_OSPI_MATCH_MODE_AND;
  sConfig.Interval = 0x10;
  sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_OSPI_AutoPolling(&hospi1_local, &sConfig,
                           HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* ============================================================================
 * OSPI_AutoPollingMemReady
 * ============================================================================
 */
static HAL_StatusTypeDef OSPI_AutoPollingMemReady(uint32_t Timeout) {
  OSPI_RegularCmdTypeDef sCommand = {0};
  OSPI_AutoPollingTypeDef sConfig = {0};

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = READ_STATUS_REG_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
  sCommand.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.NbData = 1;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  sConfig.Match = 0x00;
  sConfig.Mask = STATUS_REG_WIP_MASK;
  sConfig.MatchMode = HAL_OSPI_MATCH_MODE_AND;
  sConfig.Interval = 0x10;
  sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_OSPI_AutoPolling(&hospi1_local, &sConfig, Timeout) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* ============================================================================
 * OSPI_ResetMemory
 * ============================================================================
 */
/* ============================================================================
 * OSPI_QPI_Reset
 * ============================================================================
 */
static HAL_StatusTypeDef OSPI_QPI_Reset(void) {
  OSPI_RegularCmdTypeDef sCommand = {0};

  /* 1. Reset Enable (0x66) in QPI (4-line) mode */
  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = RESET_ENABLE_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_4_LINES; /* QPI Mode */
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* 타임아웃은 짧게 설정 (QPI 모드가 아니면 응답 안 할 수 있음) */
  HAL_OSPI_Command(&hospi1_local, &sCommand, 10);

  /* 2. Reset (0x99) in QPI (4-line) mode */
  sCommand.Instruction = RESET_MEMORY_CMD;

  HAL_OSPI_Command(&hospi1_local, &sCommand, 10);

  return HAL_OK;
}

static HAL_StatusTypeDef OSPI_ResetMemory(void) {
  OSPI_RegularCmdTypeDef sCommand = {0};

  sCommand.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
  sCommand.Instruction = RESET_ENABLE_CMD;
  sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode = HAL_OSPI_DATA_NONE;
  sCommand.DummyCycles = 0;
  sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
  sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  sCommand.Instruction = RESET_MEMORY_CMD;

  if (HAL_OSPI_Command(&hospi1_local, &sCommand,
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return HAL_ERROR;
  }

  HAL_Delay(30);

  return HAL_OK;
}
