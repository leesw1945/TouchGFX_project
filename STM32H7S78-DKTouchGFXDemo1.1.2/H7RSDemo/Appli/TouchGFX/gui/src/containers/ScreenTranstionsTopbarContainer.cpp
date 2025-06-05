#include <gui/containers/ScreenTranstionsTopbarContainer.hpp>
#include "gui/common/constans.hpp"

ScreenTranstionsTopbarContainer::ScreenTranstionsTopbarContainer()
{

}

void ScreenTranstionsTopbarContainer::initialize()
{
    ScreenTranstionsTopbarContainerBase::initialize();

    if (chromArtButton.getState())
    {
        chromArtGradient.startFadeAnimation(255, CHROM_ART_GRADIENT_FADE_DURATION);
    }
}

void ScreenTranstionsTopbarContainer::chromARTStateChangedAction(bool state)
{
    if (state)
    {
        chromArtGradient.startFadeAnimation(255, CHROM_ART_GRADIENT_FADE_DURATION);
    }
    else
    {
        chromArtGradient.startFadeAnimation(0, CHROM_ART_GRADIENT_FADE_DURATION);
    }
}

void ScreenTranstionsTopbarContainer::animationStateChanged(bool state)
{
    chromArtButton.setTouchable(!state);
}

void ScreenTranstionsTopbarContainer::updateMCU(uint16_t value)
{
    Unicode::snprintf(mcuValueBuffer, MCUVALUE_SIZE, "%d", value);
    mcuValue.invalidate();
}

void ScreenTranstionsTopbarContainer::updateFPS(uint16_t value)
{
    Unicode::snprintf(fpsValueBuffer, FPSVALUE_SIZE, "%d", value);
    fpsValue.invalidate();
}
