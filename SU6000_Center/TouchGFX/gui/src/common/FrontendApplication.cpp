#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/touch_calibration_screen/Touch_CalibrationPresenter.hpp>
#include <gui/touch_calibration_screen/Touch_CalibrationView.hpp>
#include <touchgfx/transitions/NoTransition.hpp>

#ifndef SIMULATOR
extern "C" {
#include "touch_calibration.h"
}
#endif

FrontendApplication::FrontendApplication(Model &m, FrontendHeap &heap)
    : FrontendApplicationBase(m, heap) {}

void FrontendApplication::changeToStartScreen() {
#ifndef SIMULATOR
  g_calibValid = Calib_Load(&g_calibData);
  if (g_calibValid) {
    static_cast<FrontendHeapBase &>(frontendHeap).gotoStartScreen(*this);
  } else {
    gotoTouch_CalibrationScreenNoTransition();
  }
#else
  static_cast<FrontendHeapBase &>(frontendHeap).gotoStartScreen(*this);
#endif
}

void FrontendApplication::gotoTouch_CalibrationScreenNoTransition() {
  calibTransitionCallback = touchgfx::Callback<FrontendApplication>(
      this, &FrontendApplication::gotoTouch_CalibrationScreenNoTransitionImpl);
  pendingScreenTransitionCallback = &calibTransitionCallback;
}

void FrontendApplication::gotoTouch_CalibrationScreenNoTransitionImpl() {
  touchgfx::makeTransition<Touch_CalibrationView, Touch_CalibrationPresenter,
                           touchgfx::NoTransition, Model>(
      &currentScreen, &currentPresenter, frontendHeap, &currentTransition,
      &model);
}
