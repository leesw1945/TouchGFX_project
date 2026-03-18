#ifndef TOUCH_CALIBRATIONVIEW_HPP
#define TOUCH_CALIBRATIONVIEW_HPP

#include <gui_generated/touch_calibration_screen/Touch_CalibrationViewBase.hpp>
#include <gui/touch_calibration_screen/Touch_CalibrationPresenter.hpp>
#include <touchgfx/hal/Types.hpp>

class Touch_CalibrationView : public Touch_CalibrationViewBase
{
public:
    Touch_CalibrationView();
    virtual ~Touch_CalibrationView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    // TouchGFX가 터치 이벤트를 이 함수로 전달
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);

protected:
    static const int NUM_POINTS = 4;

    // 4개 타겟의 중심 좌표
    static const int targetCenterX[NUM_POINTS];
    static const int targetCenterY[NUM_POINTS];

    int currentPoint;                   // 현재 몇 번째 점 (0~3)
    int rawSamples[NUM_POINTS][2];      // 각 점의 raw X, Y 값

    void showTarget(int index);         // 크로스헤어를 해당 위치로 이동
    void calculateAndSave();            // 보정값 계산 + Flash 저장 + Main 이동
};

#endif // TOUCH_CALIBRATIONVIEW_HPP
