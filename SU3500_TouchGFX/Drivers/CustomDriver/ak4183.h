/**
 * @file AK4183.h
 * @brief AK4183 저항식 터치 컨트롤러 드라이버 헤더
 * @author
 * @date 2025-07-14
 *
 * @details
 * STM32F429IGT6 + AK4183 터치 컨트롤러 인터럽트 방식 구현
 * - I2C2 사용 (SCL: PH4, SDA: PH5)
 * - PENIRQN: PE3 (Falling edge interrupt)
 * - TouchGFX 통합 지원
 */

#ifndef AK4183_H_
#define AK4183_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* Defines -------------------------------------------------------------------*/
// AK4183 I2C 주소 (CAD0 핀 상태에 따라 결정)
#define AK4183_I2C_ADDR_CAD0_LOW    0x48  // CAD0 = 0
#define AK4183_I2C_ADDR_CAD0_HIGH   0x49  // CAD0 = 1

// 기본 I2C 주소 (회로도에서 CAD0 상태 확인 필요)
#define AK4183_I2C_ADDR             AK4183_I2C_ADDR_CAD0_LOW

// AK4183 컨트롤 명령어 비트 정의
#define AK4183_CMD_START_BIT        0x80  // S bit: 1=Normal, 0=Sleep
#define AK4183_CMD_12BIT_MODE       0x00  // MODE bit: 0=12bit
#define AK4183_CMD_8BIT_MODE        0x02  // MODE bit: 1=8bit
#define AK4183_CMD_PD0_ENABLE       0x00  // PD0 bit: 0=Pen interrupt enable
#define AK4183_CMD_PD0_DISABLE      0x04  // PD0 bit: 1=Power down

// 채널 선택 비트 (A2-A0)
#define AK4183_CMD_MEASURE_X        0x10  // 100: X축 측정
#define AK4183_CMD_MEASURE_Y        0x20  // 101: Y축 측정
#define AK4183_CMD_MEASURE_Z1       0x30  // 110: Z1 압력 측정
#define AK4183_CMD_MEASURE_Z2       0x38  // 111: Z2 압력 측정

// 가속 모드 (빠른 드라이버 스위칭)
#define AK4183_CMD_ACCEL_X_DRIVER   0x00  // 000: X 드라이버 가속
#define AK4183_CMD_ACCEL_Y_DRIVER   0x08  // 001: Y 드라이버 가속

// 통합 명령어 (자주 사용하는 조합)
#define AK4183_CMD_READ_X_12BIT     (AK4183_CMD_START_BIT | AK4183_CMD_MEASURE_X | AK4183_CMD_12BIT_MODE | AK4183_CMD_PD0_ENABLE)
#define AK4183_CMD_READ_Y_12BIT     (AK4183_CMD_START_BIT | AK4183_CMD_MEASURE_Y | AK4183_CMD_12BIT_MODE | AK4183_CMD_PD0_ENABLE)
#define AK4183_CMD_READ_Z1_12BIT    (AK4183_CMD_START_BIT | AK4183_CMD_MEASURE_Z1 | AK4183_CMD_12BIT_MODE | AK4183_CMD_PD0_ENABLE)
#define AK4183_CMD_READ_Z2_12BIT    (AK4183_CMD_START_BIT | AK4183_CMD_MEASURE_Z2 | AK4183_CMD_12BIT_MODE | AK4183_CMD_PD0_ENABLE)
#define AK4183_CMD_SLEEP            0x70  // Sleep mode command

// 타이밍 정의 (us)
#define AK4183_CONVERSION_TIME_US   120   // 변환 시간 (데이터시트 참조)
#define AK4183_SETTLING_TIME_US     10    // 안정화 시간

// 터치 검출 임계값
#define AK4183_TOUCH_THRESHOLD_MIN  100   // 최소 터치 압력
#define AK4183_TOUCH_THRESHOLD_MAX  3900  // 최대 유효 값 (12bit: 4095)

// 필터링 파라미터
#define AK4183_SAMPLE_COUNT         4     // 평균을 위한 샘플 수
#define AK4183_MAX_DELTA            100   // 노이즈 제거를 위한 최대 변화량

/* Type Definitions ----------------------------------------------------------*/
// AK4183 상태 열거형
typedef enum {
    AK4183_OK = 0,
    AK4183_ERROR,
    AK4183_BUSY,
    AK4183_TIMEOUT,
    AK4183_ERROR_I2C,
    AK4183_ERROR_INVALID_PARAM
} AK4183_Status_t;

// 터치 상태 열거형
typedef enum {
    AK4183_TOUCH_RELEASED = 0,
    AK4183_TOUCH_PRESSED
} AK4183_TouchState_t;

// 터치 좌표 구조체
typedef struct {
    uint16_t x;         // X 좌표 (0-4095 for 12bit)
    uint16_t y;         // Y 좌표 (0-4095 for 12bit)
    uint16_t z1;        // 압력 Z1
    uint16_t z2;        // 압력 Z2
    uint16_t pressure;  // 계산된 압력 값
    AK4183_TouchState_t state;  // 터치 상태
} AK4183_TouchData_t;

// AK4183 핸들 구조체
typedef struct {
    I2C_HandleTypeDef *hi2c;        // I2C 핸들
    uint8_t i2c_addr;               // I2C 주소

    // 터치 데이터
    AK4183_TouchData_t touch_data;  // 현재 터치 데이터
    AK4183_TouchData_t last_data;   // 이전 터치 데이터 (필터링용)

    // 상태 플래그
    volatile bool is_touched;       // 터치 상태 (인터럽트에서 설정)
    volatile bool data_ready;       // 데이터 준비 완료
    volatile bool is_busy;          // 변환 중

    // 필터링
    uint16_t x_buffer[AK4183_SAMPLE_COUNT];  // X 좌표 버퍼
    uint16_t y_buffer[AK4183_SAMPLE_COUNT];  // Y 좌표 버퍼
    uint8_t sample_index;           // 현재 샘플 인덱스

    // 콜백 함수 포인터
    void (*touch_detected_callback)(AK4183_TouchData_t *data);
    void (*touch_released_callback)(void);

} AK4183_Handle_t;

/* Function Prototypes -------------------------------------------------------*/
// 초기화/종료
AK4183_Status_t AK4183_Init(AK4183_Handle_t *handle, I2C_HandleTypeDef *hi2c);
AK4183_Status_t AK4183_DeInit(AK4183_Handle_t *handle);

// 터치 데이터 읽기
AK4183_Status_t AK4183_ReadTouch(AK4183_Handle_t *handle, AK4183_TouchData_t *data);
AK4183_Status_t AK4183_ReadTouchIT(AK4183_Handle_t *handle);  // 인터럽트 모드

// 개별 축 읽기
AK4183_Status_t AK4183_ReadX(AK4183_Handle_t *handle, uint16_t *x);
AK4183_Status_t AK4183_ReadY(AK4183_Handle_t *handle, uint16_t *y);
AK4183_Status_t AK4183_ReadPressure(AK4183_Handle_t *handle, uint16_t *z1, uint16_t *z2);

// 전원 관리
AK4183_Status_t AK4183_Sleep(AK4183_Handle_t *handle);
AK4183_Status_t AK4183_Wakeup(AK4183_Handle_t *handle);

// 인터럽트 핸들러
void AK4183_IRQHandler(AK4183_Handle_t *handle);  // EXTI 인터럽트에서 호출

// 유틸리티 함수
bool AK4183_IsTouched(AK4183_Handle_t *handle);
uint16_t AK4183_CalculatePressure(uint16_t z1, uint16_t z2, uint16_t x);

// 콜백 등록
void AK4183_RegisterTouchDetectedCallback(AK4183_Handle_t *handle,
                                         void (*callback)(AK4183_TouchData_t *));
void AK4183_RegisterTouchReleasedCallback(AK4183_Handle_t *handle,
                                         void (*callback)(void));

// 디버그 함수
#ifdef AK4183_DEBUG
void AK4183_PrintTouchData(AK4183_TouchData_t *data);
#endif

#ifdef __cplusplus
}
#endif

#endif /* AK4183_H_ */
