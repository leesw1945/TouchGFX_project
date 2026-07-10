#include <gui/batteryscreen_screen/BatteryScreenView.hpp>
#include <gui/batteryscreen_screen/BatteryScreenPresenter.hpp>

BatteryScreenPresenter::BatteryScreenPresenter(BatteryScreenView& v)
    : view(v)
{

}

void BatteryScreenPresenter::activate()
{

}

void BatteryScreenPresenter::deactivate()
{

}

uint16_t BatteryScreenPresenter::getBatteryLevel()
{
    return model->getBatteryLevel();
}

void BatteryScreenPresenter::setAmbientLightRGB(uint8_t red, uint8_t green, uint8_t blue)
{
    model->setAmbientLightRGB(red, green, blue);
}
