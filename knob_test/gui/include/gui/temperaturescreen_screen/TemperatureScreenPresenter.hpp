#ifndef TEMPERATURESCREENPRESENTER_HPP
#define TEMPERATURESCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TemperatureScreenView;

class TemperatureScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TemperatureScreenPresenter(TemperatureScreenView& v);

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

    virtual ~TemperatureScreenPresenter() {}

    void setTemperature(int value);
    int getTemperature();

    void setAmbientLightRGB(uint8_t red, uint8_t green, uint8_t blue);

private:
    TemperatureScreenPresenter();

    TemperatureScreenView& view;
};

#endif // TEMPERATURESCREENPRESENTER_HPP
