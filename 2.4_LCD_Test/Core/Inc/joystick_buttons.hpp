/**
  ******************************************************************************
  * @file    joystick_buttons.hpp
  * @brief   GFX01M2 조이스틱 → TouchGFX 하드웨어 버튼 브리지
  *
  *          사용자 파일 - CubeMX/Designer가 건드리지 않음.
  *
  *          TouchGFX의 "Hardware button is clicked" 인터랙션은 '키 번호'만
  *          알고 있고, 그 번호를 실제 GPIO에서 만들어 보내는 것이 이 클래스다.
  *          HAL이 매 프레임 sample()을 호출하고, true를 리턴하면 그 키 값이
  *          활성 스크린의 handleKeyEvent()로 전달된다.
  *
  *          키 매핑 (Designer 인터랙션의 키 값과 일치해야 함):
  *            0 = CENTER(누름), 1 = UP, 2 = DOWN, 3 = LEFT, 4 = RIGHT
  ******************************************************************************
  */
#ifndef JOYSTICK_BUTTONS_HPP
#define JOYSTICK_BUTTONS_HPP

#include <platform/driver/button/ButtonController.hpp>

class JoystickButtonController : public touchgfx::ButtonController
{
public:
    virtual void init();
    virtual bool sample(uint8_t& key);
    virtual void reset();

private:
    static const uint8_t BTN_COUNT      = 5;
    static const uint8_t DEBOUNCE_TICKS = 2;   /* 2틱(~26ms) 연속 같은 값일 때 확정 */

    uint8_t stable[BTN_COUNT];   /* 확정된 상태 (1=눌림) */
    uint8_t candCnt[BTN_COUNT];  /* 반대 상태가 연속 관측된 횟수 */
};

#endif /* JOYSTICK_BUTTONS_HPP */
