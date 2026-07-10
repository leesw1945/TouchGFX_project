#ifndef GAUGESCREENPRESENTER_HPP
#define GAUGESCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class GaugeScreenView;

class GaugeScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    GaugeScreenPresenter(GaugeScreenView& v);

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

    virtual ~GaugeScreenPresenter() {}

    void setPressure(int value);
    int getPressure();

    void setAmbientLightRGB(uint8_t red, uint8_t green, uint8_t blue);

private:
    GaugeScreenPresenter();

    GaugeScreenView& view;
};

#endif // GAUGESCREENPRESENTER_HPP
