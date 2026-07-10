#ifndef TEMPERATURESCREENVIEW_HPP
#define TEMPERATURESCREENVIEW_HPP

#include <gui_generated/temperaturescreen_screen/TemperatureScreenViewBase.hpp>
#include <gui/temperaturescreen_screen/TemperatureScreenPresenter.hpp>

class TemperatureScreenView : public TemperatureScreenViewBase
{
public:
    TemperatureScreenView();
    virtual ~TemperatureScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

    virtual void increaseTemperature();
    virtual void decreaseTemperature();

protected:
    const uint16_t MIN_VALUE = 5;
    const uint16_t MAX_VALUE = 35;
    const uint16_t FILL_AREA_SIZE = 240;
    const uint16_t LINES_PER_DEGREE = FILL_AREA_SIZE / (MAX_VALUE - MIN_VALUE);

    void changeTemperature(const int16_t change);
    void updateFill(const int16_t newLevel);

    bool isAnimating = false;
};

#endif // TEMPERATURESCREENVIEW_HPP
