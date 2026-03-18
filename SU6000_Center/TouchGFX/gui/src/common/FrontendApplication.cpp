#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <touchgfx/transitions/NoTransition.hpp>
#include <gui/touch_calibration_screen/Touch_CalibrationView.hpp>
#include <gui/touch_calibration_screen/Touch_CalibrationPresenter.hpp>

#ifndef SIMULATOR
extern "C" {
#include "touch_calibration.h"
}
#endif

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{
}

void FrontendApplication::changeToStartScreen()
{
#ifndef SIMULATOR
    if (g_calibValid) {
        gotoScreen1ScreenNoTransition();
    } else {
        gotoTouch_CalibrationScreenNoTransition();
    }
#else
    gotoScreen1ScreenNoTransition();
#endif
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
