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

// C 헤더를 C++에서 사용
extern "C" {
#include "main.h"
}

/* Private defines -----------------------------------------------------------*/
// LCD 화면 크기 (실제 디스플레이 크기)
#define LCD_WIDTH           800
#define LCD_HEIGHT          480

// 터치 캘리브레이션 값 (실제 터치 패널에 맞게 조정 필요)
#define TOUCH_CALIB_X_MIN   200    // X축 최소값
#define TOUCH_CALIB_X_MAX   3900   // X축 최대값
#define TOUCH_CALIB_Y_MIN   200    // Y축 최소값
#define TOUCH_CALIB_Y_MAX   3900   // Y축 최대값

// 터치 방향 설정 (필요시 조정)
#define TOUCH_INVERT_X      false  // X축 반전 여부
#define TOUCH_INVERT_Y      false  // Y축 반전 여부

/* Private variables ---------------------------------------------------------*/
static STM32TouchController* touchController_instance = nullptr;
static AK4183_Handle_t* ak4183_handle = nullptr;
static volatile bool new_touch_event = false;
static volatile int32_t touch_x = 0;
static volatile int32_t touch_y = 0;
static volatile bool touch_pressed = false;

/* Private function prototypes -----------------------------------------------*/
static void Touch_Event_Callback(AK4183_TouchData_t* data);
static void Touch_Release_Callback(void);
static int32_t Convert_X_Coordinate(uint16_t touch_x);
static int32_t Convert_Y_Coordinate(uint16_t touch_y);

/* Public functions ----------------------------------------------------------*/
/**
 * @brief STM32TouchController 생성자
 */
STM32TouchController::STM32TouchController()
    : last_x(0), last_y(0), touch_detected(false)
{
    touchController_instance = this;
    ak4183_handle = AK4183_GetHandle();

    // 터치 드라이버 초기화는 main.c에서 수행됨
    // 여기서는 콜백만 등록
    if (ak4183_handle != nullptr) {
        AK4183_RegisterTouchDetectedCallback(ak4183_handle, Touch_Event_Callback);
        AK4183_RegisterTouchReleasedCallback(ak4183_handle, Touch_Release_Callback);
    }
}


/**
 * @brief 터치 컨트롤러 초기화
 */
void STM32TouchController::init()
{
    // 초기화는 main.c에서 수행되므로 여기서는 상태만 확인
    if (ak4183_handle != nullptr) {
        // 터치 드라이버가 정상적으로 초기화되었는지 확인
        touch_detected = false;
        new_touch_event = false;
    }
}

/**
 * @brief 터치 이벤트 샘플링 (TouchGFX 4.25.0 호환)
 * @param x X 좌표 저장
 * @param y Y 좌표 저장
 * @return true: 터치됨, false: 터치 안됨
 */
bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    // 새로운 터치 이벤트가 있는지 확인
    if (new_touch_event && touch_pressed) {
        x = touch_x;
        y = touch_y;

        // 좌표 저장 (다음 호출을 위해)
        last_x = x;
        last_y = y;
        touch_detected = true;

        return true;
    } else if (touch_detected && !touch_pressed) {
        // 터치 해제됨
        x = last_x;
        y = last_y;
        touch_detected = false;
        new_touch_event = false;

        return false;
    }

    return touch_detected;
}
/* Private functions ---------------------------------------------------------*/
/**
 * @brief 터치 감지 콜백 함수
 * @param data AK4183 터치 데이터
 */
static void Touch_Event_Callback(AK4183_TouchData_t* data)
{
    if (data != nullptr && touchController_instance != nullptr) {
        // 터치 좌표 변환
        int32_t lcd_x = Convert_X_Coordinate(data->x);
        int32_t lcd_y = Convert_Y_Coordinate(data->y);

        // 화면 범위 체크
        if (lcd_x >= 0 && lcd_x < LCD_WIDTH && lcd_y >= 0 && lcd_y < LCD_HEIGHT) {
            touch_x = lcd_x;
            touch_y = lcd_y;
            touch_pressed = true;
            new_touch_event = true;
        }
    }
}

/**
 * @brief 터치 해제 콜백 함수
 */
static void Touch_Release_Callback(void)
{
    if (touchController_instance != nullptr) {
        touch_pressed = false;
        new_touch_event = true;
    }
}

/**
 * @brief 터치 X 좌표를 LCD 좌표로 변환
 * @param touch_x 터치 패널 X 좌표 (0-4095)
 * @return LCD X 좌표 (0-799)
 */
static int32_t Convert_X_Coordinate(uint16_t touch_raw_x)
{
    int32_t result = touch_raw_x;

    // 캘리브레이션 적용
    if (result < TOUCH_CALIB_X_MIN) {
        result = TOUCH_CALIB_X_MIN;
    } else if (result > TOUCH_CALIB_X_MAX) {
        result = TOUCH_CALIB_X_MAX;
    }

    // 정규화 (0-1 범위)
    result = result - TOUCH_CALIB_X_MIN;

    // LCD 좌표로 스케일링
    result = (result * LCD_WIDTH) / (TOUCH_CALIB_X_MAX - TOUCH_CALIB_X_MIN);

    // 반전 처리
    if (TOUCH_INVERT_X) {
        result = LCD_WIDTH - 1 - result;
    }

    return result;
}

/**
 * @brief 터치 Y 좌표를 LCD 좌표로 변환
 * @param touch_y 터치 패널 Y 좌표 (0-4095)
 * @return LCD Y 좌표 (0-479)
 */
static int32_t Convert_Y_Coordinate(uint16_t touch_raw_y)
{
    int32_t result = touch_raw_y;

    // 캘리브레이션 적용
    if (result < TOUCH_CALIB_Y_MIN) {
        result = TOUCH_CALIB_Y_MIN;
    } else if (result > TOUCH_CALIB_Y_MAX) {
        result = TOUCH_CALIB_Y_MAX;
    }

    // 정규화 (0-1 범위)
    result = result - TOUCH_CALIB_Y_MIN;

    // LCD 좌표로 스케일링
    result = (result * LCD_HEIGHT) / (TOUCH_CALIB_Y_MAX - TOUCH_CALIB_Y_MIN);

    // 반전 처리
    if (TOUCH_INVERT_Y) {
        result = LCD_HEIGHT - 1 - result;
    }

    return result;
}

/**
 * @brief 터치 캘리브레이션 값 설정
 * @param x_min X축 최소값
 * @param x_max X축 최대값
 * @param y_min Y축 최소값
 * @param y_max Y축 최대값
 */
void STM32TouchController::setCalibration(uint16_t x_min, uint16_t x_max,
                                         uint16_t y_min, uint16_t y_max)
{
    // 실제 구현에서는 이 값들을 전역 변수나 구조체에 저장
    // 여기서는 컴파일 타임 상수로 정의했으므로 런타임에는 변경 불가
    // 필요시 동적 캘리브레이션 기능 추가
}

/**
 * @brief 터치 패널 테스트 함수
 * @note 터치 좌표 확인 및 캘리브레이션용
 */
void STM32TouchController::testTouchPanel(void)
{
    AK4183_TouchData_t touch_data;

    if (ak4183_handle != nullptr) {
        // 터치 데이터 읽기
        if (AK4183_ReadTouch(ak4183_handle, &touch_data) == AK4183_OK) {
            if (touch_data.state == AK4183_TOUCH_PRESSED) {
                // 원시 터치 좌표 출력 (디버그용)
                #ifdef AK4183_DEBUG
                // 디버거에서 확인할 수 있도록 volatile 변수 사용
                volatile uint16_t debug_x = touch_data.x;
                volatile uint16_t debug_y = touch_data.y;
                volatile uint16_t debug_pressure = touch_data.pressure;

                // 변환된 LCD 좌표
                volatile int32_t lcd_x = Convert_X_Coordinate(touch_data.x);
                volatile int32_t lcd_y = Convert_Y_Coordinate(touch_data.y);

                // 컴파일러 최적화 방지
                (void)debug_x; (void)debug_y; (void)debug_pressure;
                (void)lcd_x; (void)lcd_y;
                #endif
            }
        }
    }
}

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
