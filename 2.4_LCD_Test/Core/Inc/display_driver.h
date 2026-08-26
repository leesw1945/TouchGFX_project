/**
  ******************************************************************************
  * @file    display_driver.h
  * @brief   SPI LCD driver for TouchGFX Partial Framebuffer (GRAM display)
  *
  *          User file - not touched by CubeMX / TouchGFX Designer generation.
  *
  *          Supported controllers (select with LCD_CONTROLLER below):
  *            - ILI9341V : X-NUCLEO-GFX01M2 $AZ2 shield (DT022CTFT panel)
  *            - ST7789   : custom board 2.4" ST7789T-3 panel
  ******************************************************************************
  */
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Controller selection ------------------------------------------------------*/
#define LCD_CTRL_ILI9341V   1   /* X-NUCLEO-GFX01M2 $AZ2 (shield)  */
#define LCD_CTRL_ST7789     2   /* custom board ST7789T-3          */

#ifndef LCD_CONTROLLER
#define LCD_CONTROLLER      LCD_CTRL_ILI9341V
#endif

/* Orientation selection ------------------------------------------------------
 * Must match BOTH the TouchGFX Generator resolution (CubeMX) and the
 * Designer canvas: PORTRAIT = 240x320, LANDSCAPE = 320x240.               */
#define LCD_ORIENT_PORTRAIT  1
#define LCD_ORIENT_LANDSCAPE 2

#ifndef LCD_ORIENTATION
#define LCD_ORIENTATION     LCD_ORIENT_PORTRAIT
#endif

/* Panel geometry --------------------------------------------------------------*/
#if (LCD_ORIENTATION == LCD_ORIENT_PORTRAIT)
#define LCD_WIDTH           240U
#define LCD_HEIGHT          320U
#else
#define LCD_WIDTH           320U
#define LCD_HEIGHT          240U
#endif

/* API ------------------------------------------------------------------------*/
/* Full power-on sequence: reset pulse + controller init + display on.
 * Call once from main() after MX_TouchGFX_Init(), before the main loop. */
void LCD_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_DRIVER_H */
