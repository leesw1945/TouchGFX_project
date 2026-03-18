#include <gui/touch_calibration_screen/Touch_CalibrationView.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <string.h>

#ifndef SIMULATOR
extern "C" {
#include "touch_calibration.h"
}
extern int g_lastRawX;
extern int g_lastRawY;
#else
static int g_lastRawX = 0;
static int g_lastRawY = 0;
#endif

// 크로스헤어 이미지 크기 (64 x 63)
static const int CROSS_W = 64;
static const int CROSS_H = 63;

// 4개 타겟의 중심 좌표
const int Touch_CalibrationView::targetCenterX[NUM_POINTS] = {50, 750, 50, 750};
const int Touch_CalibrationView::targetCenterY[NUM_POINTS] = {50,  50, 430, 430};

Touch_CalibrationView::Touch_CalibrationView()
    : currentPoint(0)
{
    memset(rawSamples, 0, sizeof(rawSamples));
}

void Touch_CalibrationView::setupScreen()
{
    Touch_CalibrationViewBase::setupScreen();
    currentPoint = 0;
    showTarget(0);
}

void Touch_CalibrationView::tearDownScreen()
{
    Touch_CalibrationViewBase::tearDownScreen();
}

void Touch_CalibrationView::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    if (evt.getType() != touchgfx::ClickEvent::RELEASED) {
        return;
    }

    rawSamples[currentPoint][0] = g_lastRawX;
    rawSamples[currentPoint][1] = g_lastRawY;

    currentPoint++;

    if (currentPoint < NUM_POINTS) {
        showTarget(currentPoint);
    } else {
        calculateAndSave();
    }
}

void Touch_CalibrationView::showTarget(int index)
{
    // 1. 기존 위치 지우기 (잔상 방지)
    touch_cali_crossHair.invalidate();

    int locX = targetCenterX[index] - (CROSS_W / 2);
    int locY = targetCenterY[index] - (CROSS_H / 2);

    touch_cali_crossHair.setXY(locX, locY);
    
    // 2. 새로운 위치 그리기
    touch_cali_crossHair.invalidate();
}

void Touch_CalibrationView::calculateAndSave()
{
#ifndef SIMULATOR
    int rx_at_top    = (rawSamples[0][0] + rawSamples[1][0]) / 2;
    int rx_at_bottom = (rawSamples[2][0] + rawSamples[3][0]) / 2;

    int ry_at_left   = (rawSamples[0][1] + rawSamples[2][1]) / 2;
    int ry_at_right  = (rawSamples[1][1] + rawSamples[3][1]) / 2;
    int rx_range = rx_at_bottom - rx_at_top;
    g_calibData.raw_x_min = rx_at_top    - (int)(rx_range * 50.0f / 380.0f);
    g_calibData.raw_x_max = rx_at_bottom + (int)(rx_range * 50.0f / 380.0f);

    int ry_range = ry_at_left - ry_at_right;
    g_calibData.raw_y_min = ry_at_right - (int)(ry_range * 50.0f / 700.0f);
    g_calibData.raw_y_max = ry_at_left  + (int)(ry_range * 50.0f / 700.0f);

    Calib_Save(&g_calibData);
    g_calibValid = 1;
#endif

    static_cast<FrontendApplication*>(touchgfx::Application::getInstance())
        ->gotoMainScreenNoTransition();
}
