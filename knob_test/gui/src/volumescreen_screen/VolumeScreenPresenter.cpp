#include <gui/volumescreen_screen/VolumeScreenView.hpp>
#include <gui/volumescreen_screen/VolumeScreenPresenter.hpp>

VolumeScreenPresenter::VolumeScreenPresenter(VolumeScreenView& v)
    : view(v)
{

}

void VolumeScreenPresenter::activate()
{

}

void VolumeScreenPresenter::deactivate()
{

}

void VolumeScreenPresenter::setVolume(int value)
{
    model->setVolume(value);
}

int VolumeScreenPresenter::getVolume()
{
    return model->getVolume();
}

void VolumeScreenPresenter::setAmbientLightRGB(uint8_t red, uint8_t green, uint8_t blue)
{
    model->setAmbientLightRGB(red, green, blue);
}
