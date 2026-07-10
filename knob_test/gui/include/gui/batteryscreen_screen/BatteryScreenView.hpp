#ifndef BATTERYSCREENVIEW_HPP
#define BATTERYSCREENVIEW_HPP

#include <gui_generated/batteryscreen_screen/BatteryScreenViewBase.hpp>
#include <gui/batteryscreen_screen/BatteryScreenPresenter.hpp>

class BatteryScreenView : public BatteryScreenViewBase
{
public:
    BatteryScreenView();
    virtual ~BatteryScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

protected:
    void updateBatteryIndicator();

    const uint16_t FILL_AREA_SIZE = 240;

    int16_t oldBatteryFillLevel = 0x7FFF;
    uint16_t oldBatteryPercentage = 0xFFFF;
};

#endif // BATTERYSCREENVIEW_HPP
