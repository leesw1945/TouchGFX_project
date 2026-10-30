/**
  ******************************************************************************
  * @file    display_driver.h
  * @brief   TouchGFX Partial Framebuffer용 SPI LCD 드라이버 (GRAM 방식)
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          보드: BUCKY_KEY_DISPLAY_REV01, 2.4" ENH-TV0240A101-LCM (ST7789T3)
  *          (ILI9341V 분기는 X-NUCLEO-GFX01M2 평가 쉴드용으로 유지)
  ******************************************************************************
  */
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 컨트롤러 선택 ---------------------------------------------------------------*/
#define LCD_CTRL_ILI9341V   1   /* X-NUCLEO-GFX01M2 $AZ2 (평가 쉴드)          */
#define LCD_CTRL_ST7789     2   /* BUCKY 실보드 ENH-TV0240A101-LCM (ST7789T3) */

#ifndef LCD_CONTROLLER
#define LCD_CONTROLLER      LCD_CTRL_ST7789
#endif

/* 화면 방향 선택 ---------------------------------------------------------------
 * TouchGFX Generator 해상도(CubeMX)와 Designer 캔버스 둘 다와 일치해야 한다:
 * PORTRAIT = 240x320, LANDSCAPE = 320x240.                                  */
#define LCD_ORIENT_PORTRAIT  1
#define LCD_ORIENT_LANDSCAPE 2

#ifndef LCD_ORIENTATION
#define LCD_ORIENTATION     LCD_ORIENT_PORTRAIT
#endif

/* 패널 해상도 ------------------------------------------------------------------*/
#if (LCD_ORIENTATION == LCD_ORIENT_PORTRAIT)
#define LCD_WIDTH           240U
#define LCD_HEIGHT          320U
#else
#define LCD_WIDTH           320U
#define LCD_HEIGHT          240U
#endif

/* API ------------------------------------------------------------------------*/
/* 전원 인가 후 전체 초기화: 리셋 펄스 + 컨트롤러 init + GRAM 클리어 +
 * Display ON + 백라이트 ON.
 * main()에서 MX_TouchGFX_Init() 다음, 메인 루프 진입 전에 1회 호출. */
void LCD_Init(void);

/* LCD 백라이트 켜기/끄기 (PA1 → BSS138이 LEDK를 로우사이드 스위칭). */
void LCD_SetBacklight(uint8_t on);

/* ---- VSYNC 폴백 (안전장치) ---------------------------------------------------
 * TE 인터럽트가 LCD_VSYNC_TIMEOUT_MS 동안 없으면 SysTick 기반 60Hz 가짜 VSYNC로
 * 자동 전환해 TE 없이도 TouchGFX가 화면을 그리게 한다. TE가 다시 들어오면 해제.
 * (TE 배선/패널 문제가 있어도 화면이 멈추지 않도록 남겨둔다) */
#define LCD_VSYNC_TIMEOUT_MS   1000U
#define LCD_VSYNC_FAKE_PERIOD  16U      /* ms, 약 60Hz */

/* SysTick_Handler(1ms)에서 호출 (ISR 컨텍스트) */
void LCD_VsyncFallbackTick1ms(void);
/* 1 = 현재 가짜 VSYNC로 동작 중 */
uint8_t LCD_IsVsyncFallbackActive(void);

/* SWD 라이브 워치로 읽는 진단 카운터 (TE 정상 = te_count 증가, fake_vsync 0) */
extern volatile uint32_t diag_te_count;      /* TE 인터럽트 발생 횟수      */
extern volatile uint32_t diag_blocks_sent;   /* LCD로 전송된 픽셀 블록 수  */
extern volatile uint32_t diag_fake_vsync;    /* 가짜 VSYNC 발생 횟수       */

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_DRIVER_H */
