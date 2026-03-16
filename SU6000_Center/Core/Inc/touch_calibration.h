/*
 * touch_calibration.h
 *
 *  Created on: Mar 16, 2026
 *      Author: user
 */

#ifndef INC_TOUCH_CALIBRATION_H_
#define INC_TOUCH_CALIBRATION_H_

#include <stdint.h>

#define CALIB_FLASH_ADDR    0x083FE000  // 내부 Flash 마지막 페이지
#define CALIB_MAGIC         0xCAL1B000  // 유효성 검사 매직넘버

typedef struct {
    uint32_t magic;         // CALIB_MAGIC이면 유효한 데이터
    int16_t  raw_x_min;     // 화면 상단 raw X
    int16_t  raw_x_max;     // 화면 하단 raw X
    int16_t  raw_y_min;     // 화면 우측 raw Y
    int16_t  raw_y_max;     // 화면 좌측 raw Y
    uint32_t checksum;      // 간단한 체크섬
} TouchCalibData;

// Flash에서 캘리브레이션 데이터 읽기 (유효하면 1, 아니면 0)
uint8_t  Calib_Load(TouchCalibData* data);

// Flash에 캘리브레이션 데이터 저장
uint8_t  Calib_Save(const TouchCalibData* data);

// 전역 캘리브레이션 값 (STM32TouchController에서 사용)
extern TouchCalibData g_calibData;
extern uint8_t g_calibValid;  // 1이면 보정 완료


#endif /* INC_TOUCH_CALIBRATION_H_ */
