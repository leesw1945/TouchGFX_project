#ifndef TOUCH_CALIBRATIONPRESENTER_HPP
#define TOUCH_CALIBRATIONPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Touch_CalibrationView;

class Touch_CalibrationPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    Touch_CalibrationPresenter(Touch_CalibrationView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~Touch_CalibrationPresenter() {}

private:
    Touch_CalibrationPresenter();

    Touch_CalibrationView& view;
};

#endif // TOUCH_CALIBRATIONPRESENTER_HPP
