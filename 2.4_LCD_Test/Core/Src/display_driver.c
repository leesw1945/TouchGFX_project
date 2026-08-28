/**
  ******************************************************************************
  * @file    display_driver.c
  * @brief   SPI LCD driver for TouchGFX Partial Framebuffer (GRAM display)
  *
  *          User file - not touched by CubeMX / TouchGFX Designer generation.
  *
  *          Wiring (NUCLEO-G0B1RE + X-NUCLEO-GFX01M2, labels from CubeMX):
  *            SPI1 (PA5 SCK / PA7 MOSI)  : display data, TX via DMA1_Channel1
  *            SPI1_NCS  (PA9)            : display chip select, active low
  *            SPI1_DCX  (PB14)           : command = low / data = high
  *            DISP_NRESET (PA1)          : display reset, active low
  *            DISP_TE   (PA0, EXTI0)     : tearing effect, rising edge
  *
  *          TouchGFX contract (see TouchGFXGeneratedHAL.cpp):
  *            - implement touchgfxDisplayDriverTransmitActive()
  *            - implement touchgfxDisplayDriverTransmitBlock()
  *            - call DisplayDriver_TransferCompleteCallback() when a block
  *              transfer completes (here: SPI1 TX DMA complete interrupt)
  *            - call touchgfxSignalVSync() on every TE pulse
  ******************************************************************************
  */
#include "display_driver.h"
#include "main.h"
#include "spi.h"

/* Provided by TouchGFX generated code (TouchGFXGeneratedHAL.cpp) */
extern void DisplayDriver_TransferCompleteCallback(void);
extern void touchgfxSignalVSync(void);

/* MIPI-DCS commands common to ILI9341V and ST7789 ---------------------------*/
#define DCS_SWRESET   0x01U
#define DCS_SLPOUT    0x11U
#define DCS_NORON     0x13U
#define DCS_INVON     0x21U
#define DCS_DISPON    0x29U
#define DCS_CASET     0x2AU
#define DCS_RASET     0x2BU
#define DCS_RAMWR     0x2CU
#define DCS_TEON      0x35U
#define DCS_MADCTL    0x36U
#define DCS_COLMOD    0x3AU
#define DCS_STE       0x44U

/* ST7789 vendor registers */
#define ST7789_PORCTRL 0xB2U
#define ST7789_GCTRL   0xB7U
#define ST7789_VCOMS   0xBBU

/* MADCTL value per controller and orientation. If the image appears upside
 * down, use the flipped alternative noted next to each value. */
#if (LCD_CONTROLLER == LCD_CTRL_ILI9341V)
  #if (LCD_ORIENTATION == LCD_ORIENT_PORTRAIT)
    #define LCD_MADCTL_VALUE  0x48U   /* MX|BGR      (flipped: 0x88) */
  #else
    #define LCD_MADCTL_VALUE  0x28U   /* MV|BGR      (flipped: 0xE8) */
  #endif
#else /* ST7789 */
  #if (LCD_ORIENTATION == LCD_ORIENT_PORTRAIT)
    #define LCD_MADCTL_VALUE  0x00U   /* normal      (flipped: 0xC0) */
  #else
    #define LCD_MADCTL_VALUE  0x60U   /* MX|MV       (flipped: 0xA0) */
  #endif
#endif

static volatile int transmitting = 0;   /* a pixel block is on the SPI bus  */
static volatile int lcd_ready    = 0;   /* init done, TE pulses are valid   */

/* diagnostics (SWD로 읽는 카운터): 값이 증가하는지가 곧 그 경로의 생사 확인 */
volatile uint32_t diag_te_count     = 0; /* TE 인터럽트 발생 횟수            */
volatile uint32_t diag_blocks_sent  = 0; /* LCD로 전송된 픽셀 블록 수        */

/* Low level helpers ----------------------------------------------------------*/

/* Send one command byte (DCX low) followed by n parameter bytes (DCX high).
 * Blocking, 8-bit frames. Used for init and window setup only - pixel data
 * goes through DMA in touchgfxDisplayDriverTransmitBlock(). */
static void LCD_WriteCmd(uint8_t cmd, const uint8_t *params, uint16_t n)
{
    HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(SPI1_DCX_GPIO_Port, SPI1_DCX_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);

    if (n > 0U)
    {
        HAL_GPIO_WritePin(SPI1_DCX_GPIO_Port, SPI1_DCX_Pin, GPIO_PIN_SET);
        HAL_SPI_Transmit(&hspi1, (uint8_t *)params, n, HAL_MAX_DELAY);
    }

    HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_SET);
}

/* Switch SPI1 frame size between 8-bit (commands) and 16-bit (pixel data).
 * 16-bit frames send the high byte of each RGB565 pixel first, which is the
 * byte order both controllers expect - no software byte swap needed. */
static void LCD_SetSpiDataSize(uint32_t dataSize)
{
    if (hspi1.Init.DataSize != dataSize)
    {
        hspi1.Init.DataSize = dataSize;
        (void)HAL_SPI_Init(&hspi1);
    }
}

/* Set the GRAM write window. End coordinates are inclusive. */
static void LCD_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t xe = (uint16_t)(x + w - 1U);
    uint16_t ye = (uint16_t)(y + h - 1U);
    uint8_t col[4] = { (uint8_t)(x >> 8), (uint8_t)x, (uint8_t)(xe >> 8), (uint8_t)xe };
    uint8_t row[4] = { (uint8_t)(y >> 8), (uint8_t)y, (uint8_t)(ye >> 8), (uint8_t)ye };

    LCD_WriteCmd(DCS_CASET, col, 4);
    LCD_WriteCmd(DCS_RASET, row, 4);
}

/* Init -----------------------------------------------------------------------*/

void LCD_Init(void)
{
    /* Safe idle levels (CS deselected, DCX = data) */
    HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SPI1_DCX_GPIO_Port, SPI1_DCX_Pin, GPIO_PIN_SET);

    /* Hardware reset pulse */
    HAL_GPIO_WritePin(DISP_NRESET_GPIO_Port, DISP_NRESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(DISP_NRESET_GPIO_Port, DISP_NRESET_Pin, GPIO_PIN_SET);
    HAL_Delay(120);

#if (LCD_CONTROLLER == LCD_CTRL_ILI9341V)
    /* X-NUCLEO-GFX01M2 $AZ2 - sequence from ST's TouchGFX low-cost reference */
    LCD_WriteCmd(DCS_SLPOUT, NULL, 0);
    HAL_Delay(120);
    LCD_WriteCmd(DCS_NORON,  NULL, 0);
    LCD_WriteCmd(DCS_MADCTL, (const uint8_t[]){ LCD_MADCTL_VALUE }, 1);
    LCD_WriteCmd(DCS_COLMOD, (const uint8_t[]){ 0x55 }, 1);        /* RGB565   */
    LCD_WriteCmd(DCS_TEON,   (const uint8_t[]){ 0x00 }, 1);        /* V-blank  */
    LCD_WriteCmd(DCS_STE,    (const uint8_t[]){ 0x00, 0x00 }, 2);  /* line 0   */
#elif (LCD_CONTROLLER == LCD_CTRL_ST7789)
    /* Custom board ST7789T-3. Replace VCOMS/porch values with the init code
     * recommended by the LCD module vendor once available. */
    LCD_WriteCmd(DCS_SWRESET, NULL, 0);
    HAL_Delay(120);
    LCD_WriteCmd(DCS_SLPOUT,  NULL, 0);
    HAL_Delay(120);
    LCD_WriteCmd(DCS_MADCTL,  (const uint8_t[]){ LCD_MADCTL_VALUE }, 1); /* +0x08 if R/B swapped */
    LCD_WriteCmd(DCS_COLMOD,  (const uint8_t[]){ 0x55 }, 1);       /* RGB565   */
    LCD_WriteCmd(ST7789_PORCTRL, (const uint8_t[]){ 0x0C, 0x0C, 0x00, 0x33, 0x33 }, 5);
    LCD_WriteCmd(ST7789_GCTRL,   (const uint8_t[]){ 0x35 }, 1);
    LCD_WriteCmd(ST7789_VCOMS,   (const uint8_t[]){ 0x20 }, 1);
    LCD_WriteCmd(DCS_INVON,   NULL, 0);                            /* IPS panels need inversion */
    LCD_WriteCmd(DCS_TEON,    (const uint8_t[]){ 0x00 }, 1);
    LCD_WriteCmd(DCS_STE,     (const uint8_t[]){ 0x00, 0x00 }, 2);
#else
#error "LCD_CONTROLLER must be LCD_CTRL_ILI9341V or LCD_CTRL_ST7789"
#endif

    LCD_WriteCmd(DCS_DISPON, NULL, 0);
    HAL_Delay(20);

    lcd_ready = 1;
}

/* TouchGFX Partial Framebuffer interface -------------------------------------*/

int touchgfxDisplayDriverTransmitActive(void)
{
    return transmitting;
}

void touchgfxDisplayDriverTransmitBlock(const uint8_t *pixels,
                                        uint16_t x, uint16_t y,
                                        uint16_t w, uint16_t h)
{
    transmitting = 1;
    diag_blocks_sent++;

    LCD_SetWindow(x, y, w, h);

    /* RAMWR command in 8-bit mode, keep CS low for the pixel burst */
    uint8_t ramwr = DCS_RAMWR;
    HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SPI1_DCX_GPIO_Port, SPI1_DCX_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &ramwr, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SPI1_DCX_GPIO_Port, SPI1_DCX_Pin, GPIO_PIN_SET);

    /* Pixel burst: 16-bit frames, DMA is configured half-word in CubeMX */
    LCD_SetSpiDataSize(SPI_DATASIZE_16BIT);
    (void)HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)pixels, (uint16_t)((uint32_t)w * h));
}

/* SPI TX DMA complete. SPI1 = display pixel block finished. */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_SET);
        LCD_SetSpiDataSize(SPI_DATASIZE_8BIT);
        transmitting = 0;

        /* Hand the freed block back to TouchGFX; starts the next transfer
         * immediately if another block is already rendered. */
        DisplayDriver_TransferCompleteCallback();
    }
}

/* TE (tearing effect) rising edge = start of vertical blanking.
 * This is the TouchGFX frame tick. */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    if ((GPIO_Pin == DISP_TE_Pin) && lcd_ready)
    {
        diag_te_count++;
        touchgfxSignalVSync();
    }
}
