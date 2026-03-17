#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }

    // 시작 화면 분기: 캘리브레이션 완료 여부에 따라 다른 화면으로
    virtual void changeToStartScreen();

    // Touch_Calibration 화면으로 이동 (캘리브레이션 화면, 설정에서도 호출 가능)
    void gotoTouch_CalibrationScreenNoTransition();

private:
    touchgfx::Callback<FrontendApplication> calibTransitionCallback;
    void gotoTouch_CalibrationScreenNoTransitionImpl();
};

#endif // FRONTENDAPPLICATION_HPP
