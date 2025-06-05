#include <gui/containers/NeoChromToggleButton_E_Bike.hpp>
#include <gui/common/Utils.hpp>

namespace
{
    static const int VISIBLE = 255;
    static const int HIDDEN = 0;

    static const int HIGHEST_Y_VALUE = 54;
    static const int LOWEST_Y_VALUE = 2;
    static const int X_VALUE = 1;

    static const uint32_t PURPLE_COLOR = 0x7D8EF5;
    static const uint32_t GREEN_COLOR = 0x19FFC6;
}

NeoChromToggleButton_E_Bike::NeoChromToggleButton_E_Bike(): isNeoChromActive(true)
{

}

void NeoChromToggleButton_E_Bike::initialize()
{
    NeoChromToggleButton_E_BikeBase::initialize();
}

void NeoChromToggleButton_E_Bike::toggleNeoChrom()
{
    if (isNeoChromActive)
    {
        setButtonStateToOff();
    }
    else
    {
        setButtonStateToOn();
    }
    
    Utils::activateNeoChrom(isNeoChromActive);
}

void NeoChromToggleButton_E_Bike::setButtonStateToOn()
{
    moveIndicatorsToOnState();

    changeTextLabelColorsToOnState();

    isNeoChromActive = true;
}

void NeoChromToggleButton_E_Bike::moveIndicatorsToOnState()
{
    onIndicator.setXY(X_VALUE, HIGHEST_Y_VALUE);

    offIndicator.startFadeAnimation(HIDDEN, 15, EasingEquations::quintEaseOut);

    onIndicator.startFadeAnimation(VISIBLE, 15, EasingEquations::linearEaseIn);
    onIndicator.setMoveAnimationDelay(6);
    onIndicator.startMoveAnimation(X_VALUE, LOWEST_Y_VALUE, 30, EasingEquations::linearEaseIn, EasingEquations::cubicEaseOut);
}

void NeoChromToggleButton_E_Bike::changeTextLabelColorsToOnState()
{
    onLabel.setColor(colortype(GREEN_COLOR));
    offLabel.setColor(colortype(PURPLE_COLOR));

    invalidateTextLabels();
}

void NeoChromToggleButton_E_Bike::setButtonStateToOff()
{
    moveIndicatorsToOffState();

    changeTextLabelColorsToOffState();

    isNeoChromActive = false;
}

void NeoChromToggleButton_E_Bike::moveIndicatorsToOffState()
{
    offIndicator.setXY(X_VALUE, LOWEST_Y_VALUE);

    onIndicator.startFadeAnimation(HIDDEN, 15, EasingEquations::quintEaseOut);

    offIndicator.startFadeAnimation(VISIBLE, 15, EasingEquations::linearEaseIn);
    offIndicator.setMoveAnimationDelay(6);
    offIndicator.startMoveAnimation(X_VALUE, HIGHEST_Y_VALUE, 30, EasingEquations::linearEaseIn, EasingEquations::cubicEaseOut);
}

void NeoChromToggleButton_E_Bike::changeTextLabelColorsToOffState()
{
    onLabel.setColor(colortype(PURPLE_COLOR));
    offLabel.setColor(colortype(GREEN_COLOR));

    invalidateTextLabels();
}

void NeoChromToggleButton_E_Bike::invalidateTextLabels()
{
    onLabel.invalidate();
    offLabel.invalidate();
}