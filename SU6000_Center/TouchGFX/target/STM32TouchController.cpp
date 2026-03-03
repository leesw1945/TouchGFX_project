/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : STM32TouchController.cpp
 ******************************************************************************
 * This file was created by TouchGFX Generator 4.26.0. This file is only
 * generated once! Delete this file from your project and re-generate code
 * using STM32CubeMX or change this file manually to update it.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* USER CODE BEGIN STM32TouchController */

#include <STM32TouchController.hpp>

extern "C" {
#include "ak4183.h"
#include <stdio.h>
}

void STM32TouchController::init() {
  /**
   * Initialize touch controller and driver
   *
   */
  AK4183_Init();
}

bool STM32TouchController::sampleTouch(int32_t &x, int32_t &y) {
  /**
   * By default sampleTouch returns false,
   * return true if a touch has been detected, otherwise false.
   *
   * Coordinates are passed to the caller by reference by x and y.
   *
   * This function is called by the TouchGFX framework.
   * By default sampleTouch is called every tick, this can be adjusted by
   * HAL::setTouchSampleRate(int8_t);
   *
   */

  // DEBUG MODE: 강제로 매 프레임마다 I2C를 읽어서 터치 칩 상태 확인
  uint16_t raw_x = 0, raw_y = 0;
  int pin_state = HAL_GPIO_ReadPin(TOUCH_I2C_IRQ_GPIO_Port, TOUCH_I2C_IRQ_Pin);

  if (AK4183_ReadXY(&raw_x, &raw_y)) {
    // I2C 통신 성공 시 매번 출력 (터미널이 빠르게 올라갈 것입니다)
    // 화면에 손을 대지 않았을 때와 대었을 때 값이 변하는지 확인하세요!
    printf("IRQ Pin: %d | RAW X: %d, Y: %d\r\n", pin_state, raw_x, raw_y);

    // Map 12-bit ADC raw values (0-4095) to 800x480 screen coordinates
    x = (raw_x * 800) / 4096;
    y = (raw_y * 480) / 4096;
    return true;
  } else {
    // I2C 통신 자체가 실패한 경우
    printf("I2C Read Failed!\r\n");
  }

  return false;
}

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
