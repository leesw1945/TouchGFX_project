#ifndef CALIBRATIONVIEW_HPP
#define CALIBRATIONVIEW_HPP

#include <gui_generated/calibration_screen/CalibrationViewBase.hpp>
#include <gui/calibration_screen/CalibrationPresenter.hpp>

class CalibrationView : public CalibrationViewBase
{
public:
    CalibrationView();
    virtual ~CalibrationView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // CALIBRATIONVIEW_HPP
