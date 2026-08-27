/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @brief   STM32CubeProgrammer external loader for MX25L6433F (64Mbit SPI
  *          NOR) on STM32G0B1, plain 1-line SPI2 + GPIO chip select.
  *
  *          Ported from the working MX25L12833F_Loader (STM32U5G9 + OCTOSPI):
  *            - same command set, JEDEC ID C2 20 17 instead of C2 20 18
  *            - HAL SPI blocking transfers instead of OSPI
  *            - SysTick bypassed (HAL_InitTick/HAL_GetTick/HAL_Delay overridden)
  *            - runs at reset clock HSI16 (SPI2 = 8 Mbit/s) - no PLL needed
  *
  *          Pin map selection (LOADER_PINMAP):
  *            NUCLEO : SCK PB13(AF0) / MISO PC2(AF1) / MOSI PC3(AF1) / CS PA8
  *            CUSTOM : SCK PB13(AF0) / MISO PB14(AF0) / MOSI PB15(AF0) / CS PA8
  ******************************************************************************
  */
#include "stm32g0xx_hal.h"
#include "Dev_Inf.h"
#include <string.h>

#define KeepInCompilation __attribute__((used))

/* ============================================================================
 * Target pin map
 * ============================================================================ */
#define LOADER_PINMAP_NUCLEO   1   /* NUCLEO-G0B1RE + X-NUCLEO-GFX01M2 */
#define LOADER_PINMAP_CUSTOM   2   /* custom G0B1CBT6 board (LQFP48)   */

#ifndef LOADER_PINMAP
#define LOADER_PINMAP          LOADER_PINMAP_NUCLEO
#endif

#define FLASH_CS_PORT          GPIOA
#define FLASH_CS_PIN           GPIO_PIN_8

/* ============================================================================
 * MX25L6433F commands / masks
 * ============================================================================ */
#define CMD_RESET_ENABLE       0x66
#define CMD_RESET_MEMORY       0x99
#define CMD_READ_ID            0x9F
#define CMD_READ               0x03
#define CMD_READ_STATUS        0x05
#define CMD_WRITE_STATUS       0x01
#define CMD_READ_SECURITY      0x2B
#define CMD_WRITE_ENABLE       0x06
#define CMD_PAGE_PROG          0x02
#define CMD_SECTOR_ERASE_4K    0x20
#define CMD_CHIP_ERASE         0x60
#define CMD_GANG_BLOCK_UNLOCK  0x98

#define STATUS_WIP             0x01
#define STATUS_WEL             0x02
#define STATUS_BP_MASK         0x3C
#define SECURITY_P_FAIL        0x20
#define SECURITY_E_FAIL        0x40
#define SECURITY_WPSEL         0x80

#define PAGE_SIZE              0x100
#define SECTOR_SIZE            0x1000
#define BASE_ADDRESS           0x90000000UL
#define ADDR_MASK              0x00FFFFFFUL

#define JEDEC_MANUFACTURER     0xC2
#define JEDEC_TYPE             0x20
#define JEDEC_DENSITY_64M      0x17

/* Poll iteration budgets (one iteration ~= one RDSR transaction at 8Mbit/s) */
#define POLL_WRITE_REG         200000UL
#define POLL_PAGE_PROG         200000UL
#define POLL_SECTOR_ERASE      2000000UL
#define POLL_CHIP_ERASE        40000000UL

/* ============================================================================
 * Local state
 * ============================================================================ */
static SPI_HandleTypeDef hspi_ldr;
static uint8_t g_id[3];

/* Diagnostics: ONLY active in the standalone self-test build. Under
 * STM32CubeProgrammer any RAM we scribble on can be the tool's own data
 * buffer (observed at 0x20008000) and corrupts the flashed data. */
#define DBG_MAGIC 0x0B00B1E5UL
#define LDR_DBG ((volatile uint32_t *)0x20008000UL)
static void DbgStage(uint32_t stage, uint32_t extra)
{
#ifdef LOADER_SELFTEST
    LDR_DBG[0] = DBG_MAGIC;
    LDR_DBG[1] = stage;
    LDR_DBG[4] = extra;
#else
    (void)stage;
    (void)extra;
#endif
}

/* ============================================================================
 * HAL tick overrides - no SysTick interrupt inside an external loader
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
    /* ~16MHz core, ~4 cycles per loop iteration -> ~4000 iterations per ms */
    for (volatile uint32_t i = 0; i < (Delay * 4000U); i++)
    {
        __NOP();
    }
}

/* ============================================================================
 * SPI primitives (blocking, CS by GPIO)
 * ============================================================================ */
static inline void CS_L(void) { HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_RESET); }
static inline void CS_H(void) { HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_SET); }

static HAL_StatusTypeDef SPI_Cmd(uint8_t cmd)
{
    HAL_StatusTypeDef st;
    CS_L();
    st = HAL_SPI_Transmit(&hspi_ldr, &cmd, 1, HAL_MAX_DELAY);
    CS_H();
    return st;
}

static HAL_StatusTypeDef SPI_CmdAddr(uint8_t cmd, uint32_t addr)
{
    uint8_t hdr[4] = { cmd, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr };
    return HAL_SPI_Transmit(&hspi_ldr, hdr, 4, HAL_MAX_DELAY);   /* CS handled by caller */
}

static HAL_StatusTypeDef ReadReg(uint8_t cmd, uint8_t *val)
{
    HAL_StatusTypeDef st;
    CS_L();
    st = HAL_SPI_Transmit(&hspi_ldr, &cmd, 1, HAL_MAX_DELAY);
    if (st == HAL_OK)
    {
        st = HAL_SPI_Receive(&hspi_ldr, val, 1, HAL_MAX_DELAY);
    }
    CS_H();
    return st;
}

static HAL_StatusTypeDef WaitReady(uint32_t maxPolls)
{
    uint8_t status;
    for (uint32_t i = 0; i < maxPolls; i++)
    {
        if (ReadReg(CMD_READ_STATUS, &status) != HAL_OK)
        {
            return HAL_ERROR;
        }
        if ((status & STATUS_WIP) == 0U)
        {
            return HAL_OK;
        }
    }
    return HAL_TIMEOUT;
}

static HAL_StatusTypeDef WriteEnable(void)
{
    uint8_t status;

    if (SPI_Cmd(CMD_WRITE_ENABLE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    for (uint32_t i = 0; i < POLL_WRITE_REG; i++)
    {
        if (ReadReg(CMD_READ_STATUS, &status) != HAL_OK)
        {
            return HAL_ERROR;
        }
        if (status & STATUS_WEL)
        {
            return HAL_OK;
        }
    }
    return HAL_TIMEOUT;
}

static HAL_StatusTypeDef ClearBlockProtection(void)
{
    uint8_t status;

    if (ReadReg(CMD_READ_STATUS, &status) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if ((status & STATUS_BP_MASK) == 0U)
    {
        return HAL_OK;
    }

    if (WriteEnable() != HAL_OK)
    {
        return HAL_ERROR;
    }

    uint8_t newStatus = status & (uint8_t)~STATUS_BP_MASK;
    uint8_t frame[2] = { CMD_WRITE_STATUS, newStatus };
    CS_L();
    HAL_SPI_Transmit(&hspi_ldr, frame, 2, HAL_MAX_DELAY);
    CS_H();

    if (WaitReady(POLL_WRITE_REG) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (ReadReg(CMD_READ_STATUS, &status) != HAL_OK)
    {
        return HAL_ERROR;
    }
    return ((status & STATUS_BP_MASK) == 0U) ? HAL_OK : HAL_ERROR;
}

/* Security register (RDSCUR 0x2B) fail-bit check. A value of 0xFF means the
 * register is not supported / not answering as expected on this part - in
 * that case do NOT treat it as a failure (verification catches real errors). */
static int SecurityFailed(uint8_t failMask)
{
    uint8_t security = 0;
    if (ReadReg(CMD_READ_SECURITY, &security) != HAL_OK)
    {
        return 0;
    }
    if (security == 0xFFU)
    {
        return 0;
    }
    return (security & failMask) ? 1 : 0;
}

static HAL_StatusTypeDef GangBlockUnlock(void)
{
    if (WriteEnable() != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (SPI_Cmd(CMD_GANG_BLOCK_UNLOCK) != HAL_OK)
    {
        return HAL_ERROR;
    }
    return WaitReady(POLL_WRITE_REG);
}

/* ============================================================================
 * Hardware init: GPIO + SPI2 (reset clock HSI16 -> SPI 8 Mbit/s)
 * ============================================================================ */
static void Loader_HW_Init(void)
{
    GPIO_InitTypeDef gpio = { 0 };

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();

    /* CS: PA8, idle high */
    HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_SET);
    gpio.Pin   = FLASH_CS_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(FLASH_CS_PORT, &gpio);

    /* SCK: PB13 (AF0) - common to both pin maps */
    gpio.Pin       = GPIO_PIN_13;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Alternate = GPIO_AF0_SPI2;
    HAL_GPIO_Init(GPIOB, &gpio);

#if (LOADER_PINMAP == LOADER_PINMAP_NUCLEO)
    /* MISO PC2 / MOSI PC3 (AF1) */
    gpio.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Alternate = GPIO_AF1_SPI2;
    HAL_GPIO_Init(GPIOC, &gpio);
#else
    /* MISO PB14 / MOSI PB15 (AF0) */
    gpio.Pin       = GPIO_PIN_14 | GPIO_PIN_15;
    gpio.Alternate = GPIO_AF0_SPI2;
    HAL_GPIO_Init(GPIOB, &gpio);
#endif

    hspi_ldr.Instance               = SPI2;
    hspi_ldr.Init.Mode              = SPI_MODE_MASTER;
    hspi_ldr.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi_ldr.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi_ldr.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi_ldr.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi_ldr.Init.NSS               = SPI_NSS_SOFT;
    hspi_ldr.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  /* 16MHz/2 = 8 Mbit/s */
    hspi_ldr.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi_ldr.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi_ldr.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi_ldr.Init.CRCPolynomial     = 7;
    hspi_ldr.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
    HAL_SPI_Init(&hspi_ldr);
}

/* ============================================================================
 * External loader API (called by STM32CubeProgrammer)
 * ============================================================================ */
KeepInCompilation int Init(void)
{
    __disable_irq();
    DbgStage(0x01, __get_MSP());   /* record the stack pointer CubeProgrammer gave us */

    /* Startup code never runs here (CubeProgrammer calls Init() directly),
     * so initialize the statics we rely on explicitly. Do NOT clear the
     * whole .bss: CubeProgrammer may place its stack right after the image,
     * and wiping it crashes the loader. */
    memset(&hspi_ldr, 0, sizeof(hspi_ldr));
    memset(g_id, 0, sizeof(g_id));

    DbgStage(0x101, 0);
    SystemInit();
    DbgStage(0x102, 0);
    HAL_Init();
    DbgStage(0x103, 0);
    Loader_HW_Init();
    DbgStage(0x104, 0);
    HAL_Delay(10);
    DbgStage(2, 0);

    /* Software reset the flash into a known state */
    SPI_Cmd(CMD_RESET_ENABLE);
    SPI_Cmd(CMD_RESET_MEMORY);
    HAL_Delay(30);
    DbgStage(3, 0);

    /* Identify: expect C2 20 17 */
    {
        uint8_t cmd = CMD_READ_ID;
        CS_L();
        HAL_SPI_Transmit(&hspi_ldr, &cmd, 1, HAL_MAX_DELAY);
        HAL_SPI_Receive(&hspi_ldr, g_id, 3, HAL_MAX_DELAY);
        CS_H();
    }
#ifdef LOADER_SELFTEST
    LDR_DBG[2] = ((uint32_t)g_id[0] << 16) | ((uint32_t)g_id[1] << 8) | g_id[2];
#endif
    DbgStage(4, 0);
    if ((g_id[0] != JEDEC_MANUFACTURER) || (g_id[1] != JEDEC_TYPE) || (g_id[2] != JEDEC_DENSITY_64M))
    {
        return 0;
    }

    /* Lift write protection. 0xFF = security register not supported ->
     * fall back to the plain status-register block-protection clear. */
    {
        uint8_t security = 0;
        ReadReg(CMD_READ_SECURITY, &security);
        if ((security != 0xFFU) && (security & SECURITY_WPSEL))
        {
            if (GangBlockUnlock() != HAL_OK)
            {
                return 0;
            }
        }
        else if (ClearBlockProtection() != HAL_OK)
        {
            uint8_t st = 0;
            ReadReg(CMD_READ_STATUS, &st);
            DbgStage(5, st);
            return 0;
        }
    }

#ifdef LOADER_SELFTEST
    {
        uint8_t st = 0;
        ReadReg(CMD_READ_STATUS, &st);
        LDR_DBG[3] = st;
    }
#endif
    DbgStage(6, 0);   /* Init OK */
    return 1;
}

KeepInCompilation int Read(uint32_t Address, uint32_t Size, uint8_t *buffer)
{
    if (Address >= BASE_ADDRESS)
    {
        Address -= BASE_ADDRESS;
    }
    Address &= ADDR_MASK;

    CS_L();
    if (SPI_CmdAddr(CMD_READ, Address) != HAL_OK)
    {
        CS_H();
        return 0;
    }
    /* HAL_SPI_Receive size is uint16_t - split large reads */
    while (Size > 0U)
    {
        uint16_t chunk = (Size > 0x8000U) ? 0x8000U : (uint16_t)Size;
        if (HAL_SPI_Receive(&hspi_ldr, buffer, chunk, HAL_MAX_DELAY) != HAL_OK)
        {
            CS_H();
            return 0;
        }
        buffer += chunk;
        Size   -= chunk;
    }
    CS_H();
    return 1;
}

KeepInCompilation int Write(uint32_t Address, uint32_t Size, uint8_t *buffer)
{
    DbgStage(0x20, Address);

    if (Address >= BASE_ADDRESS)
    {
        Address -= BASE_ADDRESS;
    }
    Address &= ADDR_MASK;

    uint32_t end = Address + Size;

    while (Address < end)
    {
        uint32_t chunk = PAGE_SIZE - (Address % PAGE_SIZE);   /* stay inside one page */
        if (chunk > (end - Address))
        {
            chunk = end - Address;
        }

        if (WriteEnable() != HAL_OK)
        {
            DbgStage(0x21, Address);
            return 0;
        }

        CS_L();
        if (SPI_CmdAddr(CMD_PAGE_PROG, Address) != HAL_OK)
        {
            CS_H();
            DbgStage(0x22, Address);
            return 0;
        }
        if (HAL_SPI_Transmit(&hspi_ldr, buffer, (uint16_t)chunk, HAL_MAX_DELAY) != HAL_OK)
        {
            CS_H();
            DbgStage(0x23, Address);
            return 0;
        }
        CS_H();

        if (WaitReady(POLL_PAGE_PROG) != HAL_OK)
        {
            DbgStage(0x24, Address);
            return 0;
        }

        if (SecurityFailed(SECURITY_P_FAIL))
        {
            DbgStage(0x25, Address);
            return 0;
        }
        DbgStage(0x26, Address);

        Address += chunk;
        buffer  += chunk;
    }
    DbgStage(0x27, Address);
    return 1;
}

KeepInCompilation int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
    if (EraseStartAddress >= BASE_ADDRESS)
    {
        EraseStartAddress -= BASE_ADDRESS;
    }
    if (EraseEndAddress >= BASE_ADDRESS)
    {
        EraseEndAddress -= BASE_ADDRESS;
    }
    EraseStartAddress &= ADDR_MASK & ~(SECTOR_SIZE - 1U);
    EraseEndAddress   &= ADDR_MASK;

    DbgStage(0x10, EraseStartAddress);
    while (EraseStartAddress <= EraseEndAddress)
    {
        if (WriteEnable() != HAL_OK)
        {
            uint8_t st = 0;
            ReadReg(CMD_READ_STATUS, &st);
            DbgStage(0x11, st);
            return 0;
        }

        CS_L();
        if (SPI_CmdAddr(CMD_SECTOR_ERASE_4K, EraseStartAddress) != HAL_OK)
        {
            CS_H();
            DbgStage(0x12, EraseStartAddress);
            return 0;
        }
        CS_H();

        if (WaitReady(POLL_SECTOR_ERASE) != HAL_OK)
        {
            uint8_t st = 0;
            ReadReg(CMD_READ_STATUS, &st);
            DbgStage(0x13, st);
            return 0;
        }

        if (SecurityFailed(SECURITY_E_FAIL))
        {
            DbgStage(0x14, 0);
            return 0;
        }
        DbgStage(0x15, EraseStartAddress);

        EraseStartAddress += SECTOR_SIZE;
    }
    return 1;
}

KeepInCompilation int MassErase(uint32_t Parallelism)
{
    (void)Parallelism;

    if (WriteEnable() != HAL_OK)
    {
        return 0;
    }
    if (SPI_Cmd(CMD_CHIP_ERASE) != HAL_OK)
    {
        return 0;
    }
    if (WaitReady(POLL_CHIP_ERASE) != HAL_OK)
    {
        return 0;
    }

    if (SecurityFailed(SECURITY_E_FAIL))
    {
        return 0;
    }
    return 1;
}

KeepInCompilation uint64_t Verify(uint32_t MemoryAddr, uint32_t RAMBufferAddr,
                                  uint32_t Size, uint32_t missalignement)
{
    static uint8_t chunk[PAGE_SIZE];
    uint32_t offset  = missalignement & 0xFU;
    uint32_t flash   = MemoryAddr + offset;
    uint8_t *ram     = (uint8_t *)(RAMBufferAddr + offset);
    uint32_t remain  = Size - offset;

    while (remain > 0U)
    {
        uint32_t n = (remain > sizeof(chunk)) ? sizeof(chunk) : remain;
        if (Read(flash, n, chunk) == 0)
        {
            return (uint64_t)flash;
        }
        for (uint32_t i = 0; i < n; i++)
        {
            if (chunk[i] != ram[i])
            {
                return (uint64_t)(flash + i);
            }
        }
        flash  += n;
        ram    += n;
        remain -= n;
    }
    return (uint64_t)(MemoryAddr + Size - offset);
}

#ifdef LOADER_SELFTEST
/* Self-test firmware: runs the loader logic as a normal application and
 * leaves results at LDR_DBG (0x20008000) for SWD readout:
 *   [0]=magic [1]=last stage [2]=JEDEC [3]=status [4]=extra
 *   [5]=Init result [6]=SectorErase result [7]=Write result
 *   [8]=readback word0 (expect 0x03020100) [9]=0xC0FFEE end marker */
int main(void)
{
    static uint8_t pattern[256];
    static uint8_t readback[256];

    LDR_DBG[5] = (uint32_t)Init();

    /* incrementing byte pattern 00 01 02 ... FF */
    for (uint32_t i = 0; i < sizeof(pattern); i++)
    {
        pattern[i] = (uint8_t)i;
    }
    if (LDR_DBG[5] == 1U)
    {
        LDR_DBG[6] = (uint32_t)SectorErase(0x90000000UL, 0x90000FFFUL);
        if (LDR_DBG[6] == 1U)
        {
            LDR_DBG[7] = (uint32_t)Write(0x90000000UL, sizeof(pattern), pattern);
            if (LDR_DBG[7] == 1U)
            {
                /* [8]=Read() return, [10..13]=first 16 bytes via READ 0x03 */
                LDR_DBG[8] = (uint32_t)Read(0x90000000UL, sizeof(readback), readback);
                for (int w = 0; w < 4; w++)
                {
                    LDR_DBG[10 + w] = ((uint32_t)readback[w * 4 + 3] << 24)
                                    | ((uint32_t)readback[w * 4 + 2] << 16)
                                    | ((uint32_t)readback[w * 4 + 1] << 8)
                                    |  (uint32_t)readback[w * 4 + 0];
                }

                /* [14..15]=first 8 bytes via FAST READ 0x0B (+1 dummy) */
                {
                    uint8_t hdr[5] = { 0x0B, 0x00, 0x00, 0x00, 0x00 };
                    uint8_t fr[8] = { 0 };
                    CS_L();
                    HAL_SPI_Transmit(&hspi_ldr, hdr, 5, HAL_MAX_DELAY);
                    HAL_SPI_Receive(&hspi_ldr, fr, 8, HAL_MAX_DELAY);
                    CS_H();
                    LDR_DBG[14] = ((uint32_t)fr[3] << 24) | ((uint32_t)fr[2] << 16) | ((uint32_t)fr[1] << 8) | fr[0];
                    LDR_DBG[15] = ((uint32_t)fr[7] << 24) | ((uint32_t)fr[6] << 16) | ((uint32_t)fr[5] << 8) | fr[4];
                }

                /* [16]=status register after everything */
                {
                    uint8_t st = 0;
                    ReadReg(CMD_READ_STATUS, &st);
                    LDR_DBG[16] = st;
                }
            }
        }
    }
    LDR_DBG[9] = 0xC0FFEEUL;

    for (;;)
    {
    }
}
#else
/* Startup code jumps to main if the loader is ever started as a program */
int main(void)
{
    for (;;)
    {
    }
}
#endif
