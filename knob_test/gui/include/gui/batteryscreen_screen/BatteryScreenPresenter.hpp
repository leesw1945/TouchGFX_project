#ifndef BATTERYSCREENPRESENTER_HPP
#define BATTERYSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class BatteryScreenView;

class BatteryScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    BatteryScreenPresenter(BatteryScreenView& v);

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

    virtual ~BatteryScreenPresenter() {}

    uint16_t getBatteryLevel();
    void setAmbientLightRGB(uint8_t red, uint8_t green, uint8_t blue);

private:
    BatteryScreenPresenter();

    BatteryScreenView& view;
};

#endif // BATTERYSCREENPRESENTER_HPP
