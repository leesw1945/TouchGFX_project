#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <touchgfx/transitions/NoTransition.hpp>
#include <gui/touch_calibration_screen/Touch_CalibrationView.hpp>
#include <gui/touch_calibration_screen/Touch_CalibrationPresenter.hpp>

extern "C" {
#include "touch_calibration.h"
}

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{
}

void FrontendApplication::changeToStartScreen()
{
    if (g_calibValid) {
        gotoScreen1ScreenNoTransition();  // 보정 완료 → 기존 시작 화면
    } else {
        gotoTouch_CalibrationScreenNoTransition();  // 첫 부팅 → 보정 화면
    }
}

void FrontendApplication::gotoTouch_CalibrationScreenNoTransition()
{
    calibTransitionCallback = touchgfx::Callback<FrontendApplication>(
        this, &FrontendApplication::gotoTouch_CalibrationScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &calibTransitionCallback;
}

void FrontendApplication::gotoTouch_CalibrationScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Touch_CalibrationView, Touch_CalibrationPresenter,
        touchgfx::NoTransition, Model>(
        &currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}
