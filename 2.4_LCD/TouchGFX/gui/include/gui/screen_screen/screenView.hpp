#ifndef SCREENVIEW_HPP
#define SCREENVIEW_HPP

#include <gui_generated/screen_screen/screenViewBase.hpp>
#include <gui/screen_screen/screenPresenter.hpp>

class screenView : public screenViewBase
{
public:
    screenView();
    virtual ~screenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /* 매 프레임(TE 주기, 약 76Hz) 호출됨 - Emergency 호흡 애니메이션 담당 */
    virtual void handleTickEvent();

    /* Emergency 표시 on/off. 지금은 데모라 setupScreen에서 true로 켜지만,
     * 실제 펌웨어에서는 CAN 수신 → Model → Presenter가 이 함수를 호출하게 된다 */
    void setEmergency(bool active);

protected:
    /* 호흡(알파) 파라미터: MIN을 0이 아닌 값으로 두면 완전히 꺼지지 않고
     * "은은하게" 남는다. STEP이 클수록 빨리 깜빡인다 */
    static const int16_t EMERGENCY_ALPHA_MIN  = 25;
    static const int16_t EMERGENCY_ALPHA_MAX  = 255;
    static const int16_t EMERGENCY_ALPHA_STEP = 20;   /* 76Hz 기준 왕복 약 1.5초 */

    bool    emergencyActive;
    int16_t emergencyAlpha;      /* 현재 알파 (int16: 경계 계산 시 음수/오버 방지) */
    int16_t emergencyAlphaDir;   /* +STEP(밝아지는 중) / -STEP(어두워지는 중) */

    void applyEmergencyAlpha(uint8_t alpha);
};

#endif // SCREENVIEW_HPP
