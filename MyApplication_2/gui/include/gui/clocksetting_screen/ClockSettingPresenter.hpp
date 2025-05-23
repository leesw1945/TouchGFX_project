#ifndef CLOCKSETTINGPRESENTER_HPP
#define CLOCKSETTINGPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ClockSettingView;

class ClockSettingPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ClockSettingPresenter(ClockSettingView& v);

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

    virtual ~ClockSettingPresenter() {}

    void saveHour(int16_t hour)
    {
        model->saveHour(hour);
    }

    void saveMinute(int16_t minute)
    {
        model->saveMinute(minute);
    }

    int16_t getHour()
    {
        return model->getHour();
    }

    int16_t getMinute()
    {
        return model->getMinute();
    }

private:
    ClockSettingPresenter();

    ClockSettingView& view;
};

#endif // CLOCKSETTINGPRESENTER_HPP
