/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.25.0. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <touchgfx/hal/HAL.hpp>

// C 헤더를 C++에서 사용
extern "C" {
#include "../../Drivers/CustomDriver/ak4183.h"
#include "i2c.h"
#include "main.h"
}

/* Private variables ---------------------------------------------------------*/
// AK4183 핸들 (전역 변수로 선언 - 인터럽트에서 접근 필요)
static AK4183_Handle_t ak4183_handle;

// 터치 좌표 버퍼 (인터럽트와 메인 루프 간 데이터 전달)
static volatile int32_t touch_x = -1;
static volatile int32_t touch_y = -1;
static volatile bool touch_detected = false;
static volatile bool new_touch_data = false;

// LCD 해상도 (화면 크기)
#define LCD_WIDTH   800
#define LCD_HEIGHT  480

// AK4183 12비트 좌표를 LCD 좌표로 변환하기 위한 스케일 팩터
#define TOUCH_X_MIN  200    // 터치 패널 최소 X 값 (캘리브레이션 필요)
#define TOUCH_X_MAX  3900   // 터치 패널 최대 X 값
#define TOUCH_Y_MIN  200    // 터치 패널 최소 Y 값
#define TOUCH_Y_MAX  3900   // 터치 패널 최대 Y 값

/* Private function prototypes -----------------------------------------------*/
static void TouchDetectedCallback(AK4183_TouchData_t *data);
static void TouchReleasedCallback(void);
static int32_t MapTouchToScreen(int32_t touch_val, int32_t touch_min,
                               int32_t touch_max, int32_t screen_max);

/* TouchGFX TouchController Implementation -----------------------------------*/
void STM32TouchController::init()
{
    /**
     * 터치 컨트롤러 초기화
     * 1. AK4183 초기화
     * 2. 콜백 함수 등록
     * 3. 인터럽트 활성화
     */

    // AK4183 초기화
    AK4183_Status_t status = AK4183_Init(&ak4183_handle, &hi2c2);
    if (status != AK4183_OK) {
        // 초기화 실패 처리
        // 실제로는 에러 로깅이나 LED 표시 등을 할 수 있음
        return;
    }

    // 콜백 함수 등록
    AK4183_RegisterTouchDetectedCallback(&ak4183_handle, TouchDetectedCallback);
    AK4183_RegisterTouchReleasedCallback(&ak4183_handle, TouchReleasedCallback);

    // 변수 초기화
    touch_x = -1;
    touch_y = -1;
    touch_detected = false;
    new_touch_data = false;
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    /**
     * TouchGFX가 주기적으로 호출하는 함수
     * 터치 상태를 확인하고 좌표를 반환
     *
     * @param x 터치 X 좌표 (출력)
     * @param y 터치 Y 좌표 (출력)
     * @return true: 터치됨, false: 터치 안됨
     */

    // 인터럽트에서 설정한 터치 데이터 확인
    if (new_touch_data) {
        // 크리티컬 섹션 - 인터럽트 비활성화
        __disable_irq();

        x = touch_x;
        y = touch_y;
        bool touched = touch_detected;
        new_touch_data = false;

        __enable_irq();

        return touched;
    }

    // 새로운 데이터가 없으면 이전 상태 유지
    return touch_detected;
}

/* Private functions ---------------------------------------------------------*/
/**
 * @brief 터치 감지 콜백 함수
 * @param data AK4183에서 읽은 터치 데이터
 * @note 인터럽트 컨텍스트에서 호출됨
 */
static void TouchDetectedCallback(AK4183_TouchData_t *data)
{
    if (data->state == AK4183_TOUCH_PRESSED) {
        // AK4183 좌표를 LCD 좌표로 변환
        int32_t lcd_x = MapTouchToScreen(data->x, TOUCH_X_MIN, TOUCH_X_MAX, LCD_WIDTH);
        int32_t lcd_y = MapTouchToScreen(data->y, TOUCH_Y_MIN, TOUCH_Y_MAX, LCD_HEIGHT);

        // 좌표가 유효한 범위인지 확인
        if (lcd_x >= 0 && lcd_x < LCD_WIDTH && lcd_y >= 0 && lcd_y < LCD_HEIGHT) {
            touch_x = lcd_x;
            touch_y = lcd_y;
            touch_detected = true;
            new_touch_data = true;
        }
    }
}

/**
 * @brief 터치 해제 콜백 함수
 * @note 인터럽트 컨텍스트에서 호출됨
 */
static void TouchReleasedCallback(void)
{
    touch_detected = false;
    new_touch_data = true;
}

/**
 * @brief 터치 좌표를 화면 좌표로 매핑
 * @param touch_val 터치 패널에서 읽은 값
 * @param touch_min 터치 패널 최소값
 * @param touch_max 터치 패널 최대값
 * @param screen_max 화면 최대 픽셀 수
 * @return 변환된 화면 좌표
 */
static int32_t MapTouchToScreen(int32_t touch_val, int32_t touch_min,
                               int32_t touch_max, int32_t screen_max)
{
    // 선형 스케일링
    int32_t result = ((touch_val - touch_min) * screen_max) / (touch_max - touch_min);

    // 범위 제한
    if (result < 0) result = 0;
    if (result >= screen_max) result = screen_max - 1;

    return result;
}

/* 인터럽트 핸들러 (C 함수로 export) ----------------------------------------*/
extern "C" {

/**
 * @brief EXTI3 인터럽트 핸들러 (PE3 = PENIRQN)
 * @note stm32f4xx_it.c 파일에서 호출됨
 */
//void EXTI3_IRQHandler(void)
//{
//    /* USER CODE BEGIN EXTI3_IRQn 0 */
//
//    /* USER CODE END EXTI3_IRQn 0 */
//    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
//    /* USER CODE BEGIN EXTI3_IRQn 1 */
//
//    /* USER CODE END EXTI3_IRQn 1 */
//}

/**
 * @brief GPIO EXTI 콜백
 * @param GPIO_Pin 인터럽트가 발생한 핀
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_3) {  // PE3 = PENIRQN
        // AK4183 인터럽트 핸들러 호출
        AK4183_IRQHandler(&ak4183_handle);
    }
}

/**
 * @brief I2C 에러 콜백
 * @param hi2c I2C 핸들
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2) {
        // I2C 에러 처리
        // 필요시 재초기화 등을 수행
        HAL_I2C_DeInit(hi2c);
        HAL_I2C_Init(hi2c);
    }
}

} // extern "C"

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
