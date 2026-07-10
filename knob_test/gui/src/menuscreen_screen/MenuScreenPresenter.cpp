#include <gui/menuscreen_screen/MenuScreenView.hpp>
#include <gui/menuscreen_screen/MenuScreenPresenter.hpp>

MenuScreenPresenter::MenuScreenPresenter(MenuScreenView& v)
    : view(v)
{

}

void MenuScreenPresenter::activate()
{

}

void MenuScreenPresenter::deactivate()
{

}

void MenuScreenPresenter::setSelectedDemoNumber(int value)
{
    model->setSelectedDemoNumber(value);
}

int MenuScreenPresenter::getSelectedDemoNumber()
{
    return model->getSelectedDemoNumber();
}

void MenuScreenPresenter::setAmbientLightRGB(uint8_t red, uint8_t green, uint8_t blue)
{
    model->setAmbientLightRGB(red, green, blue);
}
