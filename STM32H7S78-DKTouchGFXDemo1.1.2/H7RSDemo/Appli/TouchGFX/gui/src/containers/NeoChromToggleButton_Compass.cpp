#include <gui/containers/NeoChromToggleButton_Compass.hpp>
#include <gui/common/Utils.hpp>

namespace
{
    static const int VISIBLE = 255;
    static const int HIDDEN = 0;

    static const int HIGHEST_X_VALUE = 83;
    static const int LOWEST_X_VALUE = 1;
    static const int Y_VALUE = 50;

    static const uint32_t PURPLE_COLOR = 0x899AFE;
    static const uint32_t DARKBLUE_COLOR = 0x150643;
}

NeoChromToggleButton_Compass::NeoChromToggleButton_Compass():isNeoChromActive(true)
{

}

void NeoChromToggleButton_Compass::initialize()
{
    NeoChromToggleButton_CompassBase::initialize();
}

void NeoChromToggleButton_Compass::toggleNeoChrom()
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

void NeoChromToggleButton_Compass::setButtonStateToOn()
{
    moveIndicatorsToOnState();

    changeTextLabelColorsToOnState();

    isNeoChromActive = true;
}

void NeoChromToggleButton_Compass::moveIndicatorsToOnState()
{
    onIndicator.setXY(LOWEST_X_VALUE, Y_VALUE);

    offIndicator.startFadeAnimation(HIDDEN, 15, EasingEquations::quintEaseOut);

    onIndicator.startFadeAnimation(VISIBLE, 15, EasingEquations::linearEaseIn);
    onIndicator.setMoveAnimationDelay(6);
    onIndicator.startMoveAnimation(HIGHEST_X_VALUE, Y_VALUE, 30, EasingEquations::cubicEaseOut, EasingEquations::linearEaseIn);
}

void NeoChromToggleButton_Compass::changeTextLabelColorsToOnState()
{
    onLabel.setColor(colortype(DARKBLUE_COLOR));
    offLabel.setColor(colortype(PURPLE_COLOR));

    invalidateTextLabels();
}

void NeoChromToggleButton_Compass::setButtonStateToOff()
{
    moveIndicatorsToOffState();

    changeTextLabelColorsToOffState();

    isNeoChromActive = false;
}

void NeoChromToggleButton_Compass::moveIndicatorsToOffState()
{
    offIndicator.setXY(HIGHEST_X_VALUE, Y_VALUE);

    onIndicator.startFadeAnimation(HIDDEN, 15, EasingEquations::quintEaseOut);

    offIndicator.startFadeAnimation(VISIBLE, 15, EasingEquations::linearEaseIn);
    offIndicator.setMoveAnimationDelay(6);
    offIndicator.startMoveAnimation(LOWEST_X_VALUE, Y_VALUE, 30, EasingEquations::cubicEaseOut, EasingEquations::linearEaseIn);
}

void NeoChromToggleButton_Compass::changeTextLabelColorsToOffState()
{
    onLabel.setColor(colortype(PURPLE_COLOR));
    offLabel.setColor(colortype(DARKBLUE_COLOR));

    invalidateTextLabels();
}

void NeoChromToggleButton_Compass::invalidateTextLabels()
{
    onLabel.invalidate();
    offLabel.invalidate();
}