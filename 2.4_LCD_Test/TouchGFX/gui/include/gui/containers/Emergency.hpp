#ifndef EMERGENCY_HPP
#define EMERGENCY_HPP

#include <gui_generated/containers/EmergencyBase.hpp>

class Emergency : public EmergencyBase
{
public:
    Emergency();
    virtual ~Emergency() {}

    virtual void initialize();

    /* Emergency 표시 on/off. 실제 펌웨어에선 CAN 수신 → Model → Presenter가
     * View를 통해 이 함수를 호출한다. */
    void setActive(bool active);

    /* 매 프레임 호출(부모 View의 handleTickEvent에서 전달) - 호흡 애니메이션 */
    void tick();

protected:
    /* 삼각파 호흡: MIN~MAX 왕복. 한 사이클 = 2*(MAX-MIN)/STEP 틱
     * = 2*190/4 = 95틱 ≈ 76Hz에서 약 1.25초 */
    static const int16_t ALPHA_MIN  = 40;
    static const int16_t ALPHA_MAX  = 230;
    static const int16_t ALPHA_STEP = 20;

    int16_t alpha;      /* 현재 알파 (경계 계산용 int16) */
    int16_t alphaDir;   /* +STEP(밝아짐) / -STEP(어두워짐) */

    void applyAlpha(uint8_t a);
};

#endif // EMERGENCY_HPP
