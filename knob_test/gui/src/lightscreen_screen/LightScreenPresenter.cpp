#include <gui/lightscreen_screen/LightScreenView.hpp>
#include <gui/lightscreen_screen/LightScreenPresenter.hpp>

LightScreenPresenter::LightScreenPresenter(LightScreenView& v)
    : view(v)
{

}

void LightScreenPresenter::activate()
{

}

void LightScreenPresenter::deactivate()
{

}

void LightScreenPresenter::setBrightness(int value)
{
    model->setBrightness(value);
}

int LightScreenPresenter::getBrightness()
{
    return model->getBrightness();
}

void LightScreenPresenter::setAmbientLightRGB(uint8_t red, uint8_t green, uint8_t blue)
{
    model->setAmbientLightRGB(red, green, blue);
}
