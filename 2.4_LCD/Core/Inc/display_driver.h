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

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_DRIVER_H */
