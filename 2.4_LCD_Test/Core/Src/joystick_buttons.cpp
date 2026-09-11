/**
  ******************************************************************************
  * @file    joystick_buttons.cpp
  * @brief   GFX01M2 조이스틱 → TouchGFX 하드웨어 버튼 브리지 (타깃 전용)
  *
  *          - 핀은 CubeMX 라벨(main.h) 사용: CENTER/UP/Down/LEFT/Right
  *          - 액티브 로우: 눌리면 GPIO_PIN_RESET(0)
  *          - 디바운스: 2틱 연속 같은 값일 때만 상태 확정
  *          - 에지 검출: '눌리는 순간' 1회만 키 이벤트 발생
  *            (레벨로 보내면 누르고 있는 동안 화면 전환이 연발된다)
  ******************************************************************************
  */
#include "joystick_buttons.hpp"
#include "main.h"

namespace
{
struct BtnPin
{
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint8_t       key;   /* Designer 인터랙션에 입력한 키 값 */
};

/* 키 매핑 테이블 - Designer 쪽 번호를 바꾸면 여기만 같이 바꾸면 된다 */
const BtnPin BTNS[] = {
    { CENTER_GPIO_Port, CENTER_Pin, 0 },
    { UP_GPIO_Port,     UP_Pin,     1 },
    { DOWN_GPIO_Port,   DOWN_Pin,   2 },
    { LEFT_GPIO_Port,   LEFT_Pin,   3 },
    { RIGHT_GPIO_Port,  RIGHT_Pin,  4 },
};
}

void JoystickButtonController::init()
{
    reset();
}

void JoystickButtonController::reset()
{
    for (uint8_t i = 0; i < BTN_COUNT; i++)
    {
        stable[i]  = 0;
        candCnt[i] = 0;
    }
}

bool JoystickButtonController::sample(uint8_t &key)
{
    for (uint8_t i = 0; i < BTN_COUNT; i++)
    {
        /* 액티브 로우: RESET = 눌림 */
        uint8_t raw = (HAL_GPIO_ReadPin(BTNS[i].port, BTNS[i].pin) == GPIO_PIN_RESET) ? 1 : 0;

        if (raw == stable[i])
        {
            candCnt[i] = 0;                     /* 확정 상태 유지 */
            continue;
        }
        if (++candCnt[i] < DEBOUNCE_TICKS)      /* 아직 확정 전 */
        {
            continue;
        }

        candCnt[i] = 0;
        stable[i]  = raw;                       /* 상태 전이 확정 */
        if (raw)                                /* 뗌→눌림 에지에서만 이벤트 */
        {
            key = BTNS[i].key;
            return true;
        }
    }
    return false;
}
