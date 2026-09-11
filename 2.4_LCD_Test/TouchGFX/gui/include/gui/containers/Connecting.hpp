#ifndef CONNECTING_HPP
#define CONNECTING_HPP

#include <gui_generated/containers/ConnectingBase.hpp>

class Connecting : public ConnectingBase
{
public:
    Connecting();
    virtual ~Connecting() {}

    virtual void initialize();

    /* 매 프레임 호출(부모 View의 handleTickEvent에서 전달).
     * 보이는 동안 스피너를 상시 회전시킨다. */
    void tick();

protected:
    /* 24프레임 × 15도 = 1회전. TICKS_PER_FRAME=3이면 76Hz 기준 약 1.05초/회전 */
    static const uint8_t SPINNER_FRAME_COUNT   = 24;
    static const uint8_t SPINNER_TICKS_PER_FRAME = 3;

    uint8_t spinnerFrame;
    uint8_t tickDivider;
};

#endif // CONNECTING_HPP
