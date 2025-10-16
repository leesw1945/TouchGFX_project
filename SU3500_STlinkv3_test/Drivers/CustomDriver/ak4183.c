/**
 * @file AK4183.c
 * @brief AK4183 저항식 터치 컨트롤러 드라이버 구현 (수정된 버전)
 * @author
 * @date 2025-07-14
 */

/* Includes ------------------------------------------------------------------*/
#include "../CustomDriver/ak4183.h"
#include <string.h>
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define I2C_TIMEOUT_MS      100
#define DEBOUNCE_TIME_MS    10

/* Private variables ---------------------------------------------------------*/
static AK4183_Handle_t ak4183_handle;  // 전역 핸들 정의

/* Private functions ---------------------------------------------------------*/
/**
 * @brief I2C를 통해 명령어 전송 및 데이터 읽기
 * @param handle AK4183 핸들
 * @param cmd 전송할 명령어
 * @param data 읽은 데이터를 저장할 버퍼
 * @return AK4183_Status_t
 */
static AK4183_Status_t AK4183_SendCmdAndRead(AK4183_Handle_t *handle,
                                             uint8_t cmd, uint16_t *data)
{
    HAL_StatusTypeDef hal_status;
    uint8_t rx_buffer[2];

    // 명령어 전송
    hal_status = HAL_I2C_Master_Transmit(handle->hi2c,
                                         handle->i2c_addr << 1,
                                         &cmd, 1,
                                         I2C_TIMEOUT_MS);
    if (hal_status != HAL_OK) {
        return AK4183_ERROR_I2C;
    }

    // 변환 시간 대기 (120us 필요하지만 HAL_Delay는 ms 단위)
    HAL_Delay(1);

    // 데이터 읽기 (2바이트)
    hal_status = HAL_I2C_Master_Receive(handle->hi2c,
                                        handle->i2c_addr << 1,
                                        rx_buffer, 2,
                                        I2C_TIMEOUT_MS);
    if (hal_status != HAL_OK) {
        return AK4183_ERROR_I2C;
    }

    // 12비트 데이터 조합 (MSB first)
    *data = ((uint16_t)rx_buffer[0] << 4) | (rx_buffer[1] >> 4);

    return AK4183_OK;
}

/**
 * @brief 버퍼의 평균값 계산 (노이즈 필터링)
 * @param buffer 데이터 버퍼
 * @param count 버퍼 크기
 * @return 평균값
 */
static uint16_t AK4183_CalculateAverage(uint16_t *buffer, uint8_t count)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < count; i++) {
        sum += buffer[i];
    }
    return (uint16_t)(sum / count);
}

/* Public functions ----------------------------------------------------------*/
/**
 * @brief AK4183 초기화
 * @param handle AK4183 핸들
 * @param hi2c I2C 핸들
 * @return AK4183_Status_t
 */
AK4183_Status_t AK4183_Init(AK4183_Handle_t *handle, I2C_HandleTypeDef *hi2c)
{
    if (handle == NULL || hi2c == NULL) {
        return AK4183_ERROR_INVALID_PARAM;
    }

    // 핸들 초기화
    memset(handle, 0, sizeof(AK4183_Handle_t));
    handle->hi2c = hi2c;
    handle->i2c_addr = AK4183_I2C_ADDR;

    // I2C 통신 테스트 - 간단한 명령어 전송
    uint8_t test_cmd = AK4183_CMD_READ_X_12BIT;
    HAL_StatusTypeDef hal_status = HAL_I2C_Master_Transmit(handle->hi2c,
                                                           handle->i2c_addr << 1,
                                                           &test_cmd, 1,
                                                           I2C_TIMEOUT_MS);
    if (hal_status != HAL_OK) {
        // 주소를 바꿔서 다시 시도
        handle->i2c_addr = AK4183_I2C_ADDR_CAD0_HIGH;
        hal_status = HAL_I2C_Master_Transmit(handle->hi2c,
                                             handle->i2c_addr << 1,
                                             &test_cmd, 1,
                                             I2C_TIMEOUT_MS);
        if (hal_status != HAL_OK) {
            return AK4183_ERROR_I2C;
        }
    }

    // 초기 상태 설정
    handle->is_touched = false;
    handle->data_ready = false;
    handle->is_busy = false;
    handle->sample_index = 0;

    return AK4183_OK;
}

/**
 * @brief AK4183 해제
 * @param handle AK4183 핸들
 * @return AK4183_Status_t
 */
AK4183_Status_t AK4183_DeInit(AK4183_Handle_t *handle)
{
    if (handle == NULL) {
        return AK4183_ERROR_INVALID_PARAM;
    }

    // 슬립 모드로 전환
    AK4183_Sleep(handle);

    // 핸들 초기화
    memset(handle, 0, sizeof(AK4183_Handle_t));

    return AK4183_OK;
}

/**
 * @brief X 좌표 읽기
 * @param handle AK4183 핸들
 * @param x 읽은 X 좌표
 * @return AK4183_Status_t
 */
AK4183_Status_t AK4183_ReadX(AK4183_Handle_t *handle, uint16_t *x)
{
    if (handle == NULL || x == NULL) {
        return AK4183_ERROR_INVALID_PARAM;
    }

    return AK4183_SendCmdAndRead(handle, AK4183_CMD_READ_X_12BIT, x);
}

/**
 * @brief Y 좌표 읽기
 * @param handle AK4183 핸들
 * @param y 읽은 Y 좌표
 * @return AK4183_Status_t
 */
AK4183_Status_t AK4183_ReadY(AK4183_Handle_t *handle, uint16_t *y)
{
    if (handle == NULL || y == NULL) {
        return AK4183_ERROR_INVALID_PARAM;
    }

    return AK4183_SendCmdAndRead(handle, AK4183_CMD_READ_Y_12BIT, y);
}

/**
 * @brief 압력 읽기
 * @param handle AK4183 핸들
 * @param z1 Z1 압력값
 * @param z2 Z2 압력값
 * @return AK4183_Status_t
 */
AK4183_Status_t AK4183_ReadPressure(AK4183_Handle_t *handle, uint16_t *z1, uint16_t *z2)
{
    if (handle == NULL || z1 == NULL || z2 == NULL) {
        return AK4183_ERROR_INVALID_PARAM;
    }

    AK4183_Status_t status;

    // Z1 읽기
    status = AK4183_SendCmdAndRead(handle, AK4183_CMD_READ_Z1_12BIT, z1);
    if (status != AK4183_OK) return status;

    // Z2 읽기
    status = AK4183_SendCmdAndRead(handle, AK4183_CMD_READ_Z2_12BIT, z2);
    if (status != AK4183_OK) return status;

    return AK4183_OK;
}

/**
 * @brief 터치 데이터 읽기 (폴링 방식)
 * @param handle AK4183 핸들
 * @param data 터치 데이터
 * @return AK4183_Status_t
 */
AK4183_Status_t AK4183_ReadTouch(AK4183_Handle_t *handle, AK4183_TouchData_t *data)
{
    AK4183_Status_t status;
    uint16_t x, y, z1, z2;

    if (handle == NULL || data == NULL) {
        return AK4183_ERROR_INVALID_PARAM;
    }

    // 여러 번 샘플링하여 평균값 계산 (노이즈 제거)
    for (int i = 0; i < AK4183_SAMPLE_COUNT; i++) {
        // X 좌표 읽기
        status = AK4183_ReadX(handle, &x);
        if (status != AK4183_OK) return status;
        handle->x_buffer[i] = x;

        // Y 좌표 읽기
        status = AK4183_ReadY(handle, &y);
        if (status != AK4183_OK) return status;
        handle->y_buffer[i] = y;

        // 안정화 시간
        HAL_Delay(1);
    }

    // 평균값 계산
    x = AK4183_CalculateAverage(handle->x_buffer, AK4183_SAMPLE_COUNT);
    y = AK4183_CalculateAverage(handle->y_buffer, AK4183_SAMPLE_COUNT);

    // 압력 읽기
    status = AK4183_ReadPressure(handle, &z1, &z2);
    if (status != AK4183_OK) return status;

    // 데이터 저장
    data->x = x;
    data->y = y;
    data->z1 = z1;
    data->z2 = z2;

    // 압력 계산
    data->pressure = AK4183_CalculatePressure(z1, z2, x);

    // 터치 상태 판단
    if (data->pressure > AK4183_TOUCH_THRESHOLD_MIN &&
        data->pressure < AK4183_TOUCH_THRESHOLD_MAX &&
        z1 > 50) {
        data->state = AK4183_TOUCH_PRESSED;
    } else {
        data->state = AK4183_TOUCH_RELEASED;
    }

    // 핸들에 데이터 복사
    memcpy(&handle->touch_data, data, sizeof(AK4183_TouchData_t));

    return AK4183_OK;
}

/**
 * @brief 터치 데이터 읽기 시작 (인터럽트 모드)
 * @param handle AK4183 핸들
 * @return AK4183_Status_t
 */
AK4183_Status_t AK4183_ReadTouchIT(AK4183_Handle_t *handle)
{
    if (handle == NULL || handle->is_busy) {
        return AK4183_ERROR_INVALID_PARAM;
    }

    handle->is_busy = true;
    handle->data_ready = false;

    // 간단한 폴링 방식으로 구현 (인터럽트 컨텍스트에서)
    AK4183_TouchData_t touch_data;
    AK4183_Status_t status = AK4183_ReadTouch(handle, &touch_data);

    handle->is_busy = false;

    if (status == AK4183_OK) {
        handle->data_ready = true;

        // 터치 감지 콜백 호출
        if (touch_data.state == AK4183_TOUCH_PRESSED &&
            handle->touch_detected_callback != NULL) {
            handle->touch_detected_callback(&touch_data);
        }
    }

    return status;
}

/**
 * @brief 슬립 모드로 전환
 * @param handle AK4183 핸들
 * @return AK4183_Status_t
 */
AK4183_Status_t AK4183_Sleep(AK4183_Handle_t *handle)
{
    uint8_t cmd = AK4183_CMD_SLEEP;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(handle->hi2c,
                                                       handle->i2c_addr << 1,
                                                       &cmd, 1,
                                                       I2C_TIMEOUT_MS);
    return (status == HAL_OK) ? AK4183_OK : AK4183_ERROR_I2C;
}

/**
 * @brief 정상 모드로 복귀
 * @param handle AK4183 핸들
 * @return AK4183_Status_t
 */
AK4183_Status_t AK4183_Wakeup(AK4183_Handle_t *handle)
{
    uint8_t cmd = AK4183_CMD_START_BIT | AK4183_CMD_PD0_ENABLE;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(handle->hi2c,
                                                       handle->i2c_addr << 1,
                                                       &cmd, 1,
                                                       I2C_TIMEOUT_MS);
    return (status == HAL_OK) ? AK4183_OK : AK4183_ERROR_I2C;
}

/**
 * @brief PENIRQN 인터럽트 핸들러
 * @param handle AK4183 핸들
 * @note EXTI 인터럽트 핸들러에서 호출되어야 함
 */
void AK4183_IRQHandler(AK4183_Handle_t *handle)
{
    static uint32_t last_interrupt_time = 0;
    uint32_t current_time = HAL_GetTick();

    // 디바운싱 처리
    if ((current_time - last_interrupt_time) < DEBOUNCE_TIME_MS) {
        return;
    }
    last_interrupt_time = current_time;

    // PE3 핀 상태 확인 (Low = 터치됨)
    if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3) == GPIO_PIN_RESET) {
        handle->is_touched = true;

        // 터치 데이터 읽기 시작
        AK4183_ReadTouchIT(handle);
    } else {
        handle->is_touched = false;

        // 터치 해제 콜백 호출
        if (handle->touch_released_callback != NULL) {
            handle->touch_released_callback();
        }
    }
}

/**
 * @brief 터치 상태 확인
 * @param handle AK4183 핸들
 * @return true: 터치됨, false: 터치 안됨
 */
bool AK4183_IsTouched(AK4183_Handle_t *handle)
{
    return handle->is_touched;
}

/**
 * @brief 압력 값 계산
 * @param z1 Z1 측정값
 * @param z2 Z2 측정값
 * @param x X 좌표값
 * @return 계산된 압력값
 */
uint16_t AK4183_CalculatePressure(uint16_t z1, uint16_t z2, uint16_t x)
{
    if (z1 == 0 || z1 >= z2) {
        return 0;  // 유효하지 않은 터치
    }

    // 압력 계산 공식: Rtouch = Rx * (X/4096) * (Z2/Z1 - 1)
    // 여기서는 간단히 상대적 압력값만 계산
    uint32_t pressure = (uint32_t)x * (z2 - z1) / z1;

    // 12비트 범위로 제한
    if (pressure > 4095) {
        pressure = 4095;
    }

    return (uint16_t)pressure;
}

/**
 * @brief 터치 감지 콜백 등록
 * @param handle AK4183 핸들
 * @param callback 콜백 함수 포인터
 */
void AK4183_RegisterTouchDetectedCallback(AK4183_Handle_t *handle,
                                         void (*callback)(AK4183_TouchData_t *))
{
    if (handle != NULL) {
        handle->touch_detected_callback = callback;
    }
}

/**
 * @brief 터치 해제 콜백 등록
 * @param handle AK4183 핸들
 * @param callback 콜백 함수 포인터
 */
void AK4183_RegisterTouchReleasedCallback(AK4183_Handle_t *handle,
                                         void (*callback)(void))
{
    if (handle != NULL) {
        handle->touch_released_callback = callback;
    }
}

/**
 * @brief 전역 AK4183 핸들 반환
 * @return AK4183_Handle_t*
 */
AK4183_Handle_t* AK4183_GetHandle(void)
{
    return &ak4183_handle;
}

#ifdef AK4183_DEBUG
/**
 * @brief 터치 데이터 출력 (디버그용)
 * @param data 터치 데이터
 */
void AK4183_PrintTouchData(AK4183_TouchData_t *data)
{
    printf("Touch Data:\n");
    printf("  X: %d\n", data->x);
    printf("  Y: %d\n", data->y);
    printf("  Z1: %d\n", data->z1);
    printf("  Z2: %d\n", data->z2);
    printf("  Pressure: %d\n", data->pressure);
    printf("  State: %s\n", data->state == AK4183_TOUCH_PRESSED ? "PRESSED" : "RELEASED");
}
#endif
