#ifndef TOUCH_CALIBRATIONVIEW_HPP
#define TOUCH_CALIBRATIONVIEW_HPP

#include <gui_generated/touch_calibration_screen/Touch_CalibrationViewBase.hpp>
#include <gui/touch_calibration_screen/Touch_CalibrationPresenter.hpp>

class Touch_CalibrationView : public Touch_CalibrationViewBase
{
public:
    Touch_CalibrationView();
    virtual ~Touch_CalibrationView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // TOUCH_CALIBRATIONVIEW_HPP
