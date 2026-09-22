/**
  ******************************************************************************
  * @file    display_driver.c
  * @brief   TouchGFX Partial Framebuffer용 SPI LCD 드라이버 (GRAM 방식)
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          배선 (BUCKY_KEY_DISPLAY_REV01, CubeMX 라벨 기준):
  *            SPI1 (PA5 SCK / PA7 MOSI) : 디스플레이 데이터, DMA1_Channel1로 TX
  *            SPI1_NCS   (PA4)          : 디스플레이 칩셀렉트, 액티브 로우
  *            DISP_DCX   (PA3)          : 커맨드 = Low / 데이터 = High
  *            DISP_RESET (PA2)          : 디스플레이 리셋, 액티브 로우
  *            DISP_TE    (PA8, EXTI8)   : 티어링 이펙트, 상승 에지
  *            LCD_BL     (PA1)          : 백라이트 스위치(BSS138), High = 켜짐
  *
  *          TouchGFX와의 약속 (TouchGFXGeneratedHAL.cpp 참고):
  *            - touchgfxDisplayDriverTransmitActive() 구현
  *            - touchgfxDisplayDriverTransmitBlock() 구현
  *            - 블록 전송이 끝나면 DisplayDriver_TransferCompleteCallback()
  *              호출 (여기서는 SPI1 TX DMA 완료 인터럽트에서)
  *            - TE 펄스마다 touchgfxSignalVSync() 호출
  ******************************************************************************
  */
#include "display_driver.h"
#include "main.h"
#include "spi.h"

/* TouchGFX 생성 코드가 제공하는 함수들 (TouchGFXGeneratedHAL.cpp) */
extern void DisplayDriver_TransferCompleteCallback(void);
extern void touchgfxSignalVSync(void);

/* ILI9341V와 ST7789가 공통으로 쓰는 MIPI-DCS 커맨드 ---------------------------*/
#define DCS_SWRESET   0x01U
#define DCS_SLPOUT    0x11U
#define DCS_NORON     0x13U
#define DCS_INVOFF    0x20U
#define DCS_INVON     0x21U
#define DCS_DISPON    0x29U
#define DCS_CASET     0x2AU
#define DCS_RASET     0x2BU
#define DCS_RAMWR     0x2CU
#define DCS_TEON      0x35U
#define DCS_MADCTL    0x36U
#define DCS_COLMOD    0x3AU
#define DCS_STE       0x44U

/* ST7789 벤더 레지스터 */
#define ST7789_PORCTRL   0xB2U
#define ST7789_GCTRL     0xB7U
#define ST7789_VCOMS     0xBBU
#define ST7789_LCMCTRL   0xC0U
#define ST7789_VDVVRHEN  0xC2U
#define ST7789_VRHS      0xC3U
#define ST7789_VDVS      0xC4U
#define ST7789_FRCTRL2   0xC6U
#define ST7789_PWCTRL1   0xD0U
#define ST7789_D6        0xD6U
#define ST7789_PVGAMCTRL 0xE0U
#define ST7789_NVGAMCTRL 0xE1U

/* 컨트롤러/방향별 MADCTL 값. 화면이 상하 반전되어 보이면
 * 각 값 옆에 적어둔 대체값을 사용할 것. */
#if (LCD_CONTROLLER == LCD_CTRL_ILI9341V)
  #if (LCD_ORIENTATION == LCD_ORIENT_PORTRAIT)
    #define LCD_MADCTL_VALUE  0x48U   /* MX|BGR      (반전 시: 0x88) */
  #else
    #define LCD_MADCTL_VALUE  0x28U   /* MV|BGR      (반전 시: 0xE8) */
  #endif
#else /* ST7789 */
  #if (LCD_ORIENTATION == LCD_ORIENT_PORTRAIT)
    #define LCD_MADCTL_VALUE  0x00U   /* 기본        (반전 시: 0xC0) */
  #else
    #define LCD_MADCTL_VALUE  0x60U   /* MX|MV       (반전 시: 0xA0) */
  #endif
#endif

static volatile int transmitting = 0;   /* 픽셀 블록이 SPI 버스로 나가는 중   */
static volatile int lcd_ready    = 0;   /* init 완료, TE 펄스가 유효함        */

/* 진단용 (SWD로 읽는 카운터): 값이 증가하는지가 곧 그 경로의 생사 확인 */
volatile uint32_t diag_te_count     = 0; /* TE 인터럽트 발생 횟수            */
volatile uint32_t diag_blocks_sent  = 0; /* LCD로 전송된 픽셀 블록 수        */
volatile uint32_t diag_fake_vsync   = 0; /* 가짜 VSYNC 발생 횟수             */

static volatile uint32_t te_last_ms      = 0; /* 마지막 TE 시각 (HAL_GetTick)   */
static volatile uint8_t  vsync_fallback  = 0; /* 1 = 가짜 VSYNC 모드            */
static uint16_t          fake_ms         = 0;

/* 부팅 시 GRAM 초기 클리어 색 (검정) */
#define LCD_CLEAR_COLOR        0x0000U

/* 저수준 헬퍼 -----------------------------------------------------------------*/

/* 커맨드 1바이트(DCX Low) + 파라미터 n바이트(DCX High) 전송.
 * 블로킹, 8비트 프레임. init과 윈도우 설정에만 사용 — 픽셀 데이터는
 * touchgfxDisplayDriverTransmitBlock()에서 DMA로 나간다. */
static void LCD_WriteCmd(uint8_t cmd, const uint8_t *params, uint16_t n)
{
    HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(DISP_DCX_GPIO_Port, DISP_DCX_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);

    if (n > 0U)
    {
        HAL_GPIO_WritePin(DISP_DCX_GPIO_Port, DISP_DCX_Pin, GPIO_PIN_SET);
        HAL_SPI_Transmit(&hspi1, (uint8_t *)params, n, HAL_MAX_DELAY);
    }

    HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_SET);
}

/* SPI1 프레임 크기를 8비트(커맨드)/16비트(픽셀) 사이에서 전환.
 * 16비트 프레임은 RGB565 픽셀의 상위 바이트를 먼저 내보내는데, 이게 두
 * 컨트롤러가 기대하는 바이트 순서라서 소프트웨어 바이트 스왑이 필요 없다. */
static void LCD_SetSpiDataSize(uint32_t dataSize)
{
    if (hspi1.Init.DataSize != dataSize)
    {
        hspi1.Init.DataSize = dataSize;
        (void)HAL_SPI_Init(&hspi1);
    }
}

/* GRAM 기록 윈도우 설정. 끝 좌표는 포함(inclusive)이다. */
static void LCD_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t xe = (uint16_t)(x + w - 1U);
    uint16_t ye = (uint16_t)(y + h - 1U);
    uint8_t col[4] = { (uint8_t)(x >> 8), (uint8_t)x, (uint8_t)(xe >> 8), (uint8_t)xe };
    uint8_t row[4] = { (uint8_t)(y >> 8), (uint8_t)y, (uint8_t)(ye >> 8), (uint8_t)ye };

    LCD_WriteCmd(DCS_CASET, col, 4);
    LCD_WriteCmd(DCS_RASET, row, 4);
}

/* GRAM 전체를 LCD_CLEAR_COLOR(검정)로 채워서 TouchGFX가 첫 프레임을 그리기 전까지
 * 화면에 아무것도 안 보이게 한다. 블로킹(16Mbit/s에서 약 75ms), 부팅 시 1회만. */
static void LCD_ClearScreen(void)
{
    static uint16_t zeros[LCD_WIDTH];
    uint8_t ramwr = DCS_RAMWR;

    for (uint16_t i = 0; i < LCD_WIDTH; i++)
    {
        zeros[i] = LCD_CLEAR_COLOR;
    }

    LCD_SetWindow(0, 0, LCD_WIDTH, LCD_HEIGHT);

    HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DISP_DCX_GPIO_Port, DISP_DCX_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &ramwr, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(DISP_DCX_GPIO_Port, DISP_DCX_Pin, GPIO_PIN_SET);

    LCD_SetSpiDataSize(SPI_DATASIZE_16BIT);
    for (uint16_t line = 0; line < LCD_HEIGHT; line++)
    {
        HAL_SPI_Transmit(&hspi1, (uint8_t *)zeros, LCD_WIDTH, HAL_MAX_DELAY);
    }
    LCD_SetSpiDataSize(SPI_DATASIZE_8BIT);

    HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_SET);
}

/* 초기화 ----------------------------------------------------------------------*/

void LCD_SetBacklight(uint8_t on)
{
    HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void LCD_Init(void)
{
    /* 안전한 대기 레벨 (CS 비선택, DCX = 데이터, 백라이트 꺼짐) */
    HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DISP_DCX_GPIO_Port, DISP_DCX_Pin, GPIO_PIN_SET);
    LCD_SetBacklight(0);

    /* 하드웨어 리셋 펄스 */
    HAL_GPIO_WritePin(DISP_RESET_GPIO_Port, DISP_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(DISP_RESET_GPIO_Port, DISP_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(150);

#if (LCD_CONTROLLER == LCD_CTRL_ILI9341V)
    /* X-NUCLEO-GFX01M2 $AZ2 — ST TouchGFX 저가형 레퍼런스의 시퀀스 */
    LCD_WriteCmd(DCS_SLPOUT, NULL, 0);
    HAL_Delay(120);
    LCD_WriteCmd(DCS_NORON,  NULL, 0);
    LCD_WriteCmd(DCS_MADCTL, (const uint8_t[]){ LCD_MADCTL_VALUE }, 1);
    LCD_WriteCmd(DCS_COLMOD, (const uint8_t[]){ 0x55 }, 1);        /* RGB565      */
    LCD_WriteCmd(DCS_TEON,   (const uint8_t[]){ 0x00 }, 1);        /* V-blank만   */
    LCD_WriteCmd(DCS_STE,    (const uint8_t[]){ 0x00, 0x00 }, 2);  /* 라인 0      */
#elif (LCD_CONTROLLER == LCD_CTRL_ST7789)
    /* ENH-TV0240A101-LCM 벤더 init (데이터시트 "Example Initialization Code",
     * 2025-05-21 V1.0) — MADCTL만 LCD_ORIENTATION을 따르도록 수정. */
    LCD_WriteCmd(DCS_SLPOUT,  NULL, 0);
    HAL_Delay(120);
    LCD_WriteCmd(DCS_MADCTL,  (const uint8_t[]){ LCD_MADCTL_VALUE }, 1); /* R/B 바뀌면 +0x08 */
    LCD_WriteCmd(DCS_COLMOD,  (const uint8_t[]){ 0x55 }, 1);       /* RGB565      */
    LCD_WriteCmd(DCS_TEON,    (const uint8_t[]){ 0x00 }, 1);       /* V-blank만   */
    LCD_WriteCmd(DCS_STE,     (const uint8_t[]){ 0x00, 0x20 }, 2); /* TE = 라인 32 (벤더값) */
    LCD_WriteCmd(ST7789_PORCTRL, (const uint8_t[]){ 0x0C, 0x0C, 0x00, 0x33, 0x33 }, 5);
    LCD_WriteCmd(ST7789_GCTRL,   (const uint8_t[]){ 0x75 }, 1);
    LCD_WriteCmd(ST7789_VCOMS,   (const uint8_t[]){ 0x1F }, 1);
    LCD_WriteCmd(ST7789_LCMCTRL, (const uint8_t[]){ 0x2C }, 1);
    LCD_WriteCmd(ST7789_VDVVRHEN,(const uint8_t[]){ 0x01 }, 1);
    LCD_WriteCmd(ST7789_VRHS,    (const uint8_t[]){ 0x13 }, 1);
    LCD_WriteCmd(ST7789_VDVS,    (const uint8_t[]){ 0x20 }, 1);
    LCD_WriteCmd(ST7789_FRCTRL2, (const uint8_t[]){ 0x0F }, 1);
    LCD_WriteCmd(ST7789_PWCTRL1, (const uint8_t[]){ 0xA4, 0xA1 }, 2);
    LCD_WriteCmd(ST7789_D6,      (const uint8_t[]){ 0xA1 }, 1);
    /* 벤더 예제 코드는 INVON(0x21)을 켰지만, 입고된 패널은 그 상태에서 색이 반전되어
     * 보였다(2026-09-22 실보드 확인) → 반전 끔(INVOFF). 패널 로트가 바뀌어 색이
     * 반전되면 이 한 줄만 INVON으로 되돌리면 된다. */
    LCD_WriteCmd(DCS_INVOFF,  NULL, 0);
    LCD_WriteCmd(ST7789_PVGAMCTRL,
                 (const uint8_t[]){ 0xD0, 0x08, 0x10, 0x0D, 0x0C, 0x07, 0x37,
                                    0x53, 0x4C, 0x39, 0x15, 0x15, 0x2A, 0x2D }, 14);
    LCD_WriteCmd(ST7789_NVGAMCTRL,
                 (const uint8_t[]){ 0xD0, 0x0D, 0x12, 0x08, 0x08, 0x15, 0x34,
                                    0x34, 0x4A, 0x36, 0x12, 0x13, 0x2B, 0x2F }, 14);
#else
#error "LCD_CONTROLLER must be LCD_CTRL_ILI9341V or LCD_CTRL_ST7789"
#endif

    /* 전원 인가 직후의 GRAM 노이즈가 눈에 보이지 않도록,
     * 먼저 검정으로 지운 뒤 화면을 켜고 마지막에 백라이트를 켠다. */
    LCD_ClearScreen();
    LCD_WriteCmd(DCS_DISPON, NULL, 0);
    HAL_Delay(20);
    LCD_SetBacklight(1);

    te_last_ms = HAL_GetTick();              /* 지금부터 1초 동안 TE를 기다려 본다 */
    lcd_ready = 1;
}

/* ---- VSYNC 폴백 (SysTick 1ms ISR에서 호출) ---------------------------------*/
void LCD_VsyncFallbackTick1ms(void)
{
    if (!lcd_ready)
    {
        return;
    }

    if ((HAL_GetTick() - te_last_ms) > LCD_VSYNC_TIMEOUT_MS)
    {
        /* TE가 끊겼다(또는 애초에 없다) → 16ms마다 직접 프레임 틱을 만든다 */
        vsync_fallback = 1;
        if (++fake_ms >= LCD_VSYNC_FAKE_PERIOD)
        {
            fake_ms = 0;
            diag_fake_vsync++;
            touchgfxSignalVSync();
        }
    }
    else
    {
        vsync_fallback = 0;
        fake_ms = 0;
    }
}

uint8_t LCD_IsVsyncFallbackActive(void)
{
    return vsync_fallback;
}

/* TouchGFX Partial Framebuffer 인터페이스 -------------------------------------*/

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

    /* RAMWR 커맨드는 8비트 모드로 보내고, 픽셀 버스트 동안 CS는 Low 유지 */
    uint8_t ramwr = DCS_RAMWR;
    HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DISP_DCX_GPIO_Port, DISP_DCX_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &ramwr, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(DISP_DCX_GPIO_Port, DISP_DCX_Pin, GPIO_PIN_SET);

    /* 픽셀 버스트: 16비트 프레임, DMA는 CubeMX에서 HalfWord로 설정돼 있음 */
    LCD_SetSpiDataSize(SPI_DATASIZE_16BIT);
    (void)HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)pixels, (uint16_t)((uint32_t)w * h));
}

/* SPI TX DMA 완료. SPI1 = 디스플레이 픽셀 블록 전송 끝. */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        HAL_GPIO_WritePin(SPI1_NCS_GPIO_Port, SPI1_NCS_Pin, GPIO_PIN_SET);
        LCD_SetSpiDataSize(SPI_DATASIZE_8BIT);
        transmitting = 0;

        /* 비워진 블록을 TouchGFX에 돌려준다. 다음 블록이 이미 렌더링돼
         * 있으면 곧바로 다음 전송이 시작된다. */
        DisplayDriver_TransferCompleteCallback();
    }
}

/* TE(티어링 이펙트) 상승 에지 = 수직 블랭킹 시작.
 * 이것이 TouchGFX의 프레임 틱이 된다. */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    if ((GPIO_Pin == DISP_TE_Pin) && lcd_ready)
    {
        diag_te_count++;
        te_last_ms = HAL_GetTick();
        if (!vsync_fallback)                 /* 가짜 VSYNC 중이면 중복 틱 방지 */
        {
            touchgfxSignalVSync();
        }
    }
}
