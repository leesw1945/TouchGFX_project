#include <gui/temperaturescreen_screen/TemperatureScreenView.hpp>
#include <gui/temperaturescreen_screen/TemperatureScreenPresenter.hpp>

TemperatureScreenPresenter::TemperatureScreenPresenter(TemperatureScreenView& v)
    : view(v)
{

}

void TemperatureScreenPresenter::activate()
{

}

void TemperatureScreenPresenter::deactivate()
{

}

void TemperatureScreenPresenter::setTemperature(int value)
{
    model->setTemperature(value);
}

int TemperatureScreenPresenter::getTemperature()
{
    return model->getTemperature();
}

void TemperatureScreenPresenter::setAmbientLightRGB(uint8_t red, uint8_t green, uint8_t blue)
{
    model->setAmbientLightRGB(red, green, blue);
}
