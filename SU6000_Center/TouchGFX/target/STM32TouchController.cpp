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
#include "touch_calibration.h"
#include <stdio.h>
}

// 캘리브레이션 화면에서 raw ADC 값을 읽기 위한 전역 변수
int g_lastRawX = 0;
int g_lastRawY = 0;

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

  uint16_t raw_x = 0, raw_y = 0;

  // 1. 하드웨어 인터럽트(IRQ) 핀이 눌렸을 때(0V)만 값을 읽도록 제한!
  if (HAL_GPIO_ReadPin(TOUCH_I2C_IRQ_GPIO_Port, TOUCH_I2C_IRQ_Pin) ==
      GPIO_PIN_RESET) {
    // 2. I2C 통신으로 X, Y 값을 가져옴
    if (AK4183_ReadXY(&raw_x, &raw_y)) {
      // 3. 노이즈 필터링: 터치하지 않았을 때 플로팅으로 뜨는 허수 값(4000 이상)
      // 버림
      if (raw_y > 3900 || raw_x < 100) {
        return false;
      }

      // --- [가장 중요한 캘리브레이션 구역] ---
      // 사용자가 측정한 실제 터치 패널 물리적 한계값 적용
      // X 방향(가로, 0~800)의 변화는 오히려 RAW Y값(300~3800)으로 나옴
      // Y 방향(세로, 0~480)의 변화는 오히려 RAW X값(480~3750)으로 나옴

      int raw_x_min, raw_x_max, raw_y_min, raw_y_max;

      if (g_calibValid) {
        // Flash에서 읽어온 보정값 사용
        raw_x_min = g_calibData.raw_x_min;
        raw_x_max = g_calibData.raw_x_max;
        raw_y_min = g_calibData.raw_y_min;
        raw_y_max = g_calibData.raw_y_max;
      } else {
        // 보정 전 기본값 (캘리브레이션 화면 터치용)
        raw_x_min = 480;
        raw_x_max = 3780;
        raw_y_min = 300;
        raw_y_max = 3800;
      }

      // 캘리브레이션 화면에서 raw 값을 읽을 수 있도록 저장
      g_lastRawX = raw_x;
      g_lastRawY = raw_y;

      // 측정된 범위를 벗어나는 값이 들어오면 잘라냄(Clamp)
      if (raw_x < raw_x_min)
        raw_x = raw_x_min;
      if (raw_x > raw_x_max)
        raw_x = raw_x_max;
      if (raw_y < raw_y_min)
        raw_y = raw_y_min;
      if (raw_y > raw_y_max)
        raw_y = raw_y_max;

      // 12-bit ADC -> 800x480 화면 좌표계 매핑 (축 뒤집힘 및 방향 캘리브레이션
      // 완료)

      // LCD의 X(가로 0~800)는 RAW_Y가 담당. 방향은 반대 (값이 클수록 0,
      // 작을수록 800)
      x = ((raw_y_max - raw_y) * 800) / (raw_y_max - raw_y_min);

      // LCD의 Y(세로 0~480)는 RAW_X가 담당. 방향은 정방향 (값이 작을수록 0,
      // 클수록 480)
      y = ((raw_x - raw_x_min) * 480) / (raw_x_max - raw_x_min);

      // 디버그용 출력 (터미널에 보여주기 위함)
      static int touch_counter = 0;
      if (++touch_counter >=
          5) { // 너무 빠르게 올라가는 것을 방지 (약 10Hz 로 출력)
        printf("Screen X: %ld, Y: %ld (RAW X:%u Y:%u)\r\n", x, y, raw_x, raw_y);
        touch_counter = 0;
      }

      return true; // 정상 터치 감지 및 좌표 변환 완료
    }
  }

  // 터치를 안 했거나, 값이 튀었을 때는 무조건 false를 반환해야 TouchGFX 엔진이
  // 버튼을 누르지 않습니다.
  return false;
}

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
