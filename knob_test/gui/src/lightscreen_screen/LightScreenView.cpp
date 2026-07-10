#include <gui/lightscreen_screen/LightScreenView.hpp>

LightScreenView::LightScreenView()
{

}

void LightScreenView::setupScreen()
{
    LightScreenViewBase::setupScreen();

    presenter->setAmbientLightRGB(255, 128, 0);

    Unicode::snprintf(lightTextAreaBuffer, LIGHTTEXTAREA_SIZE, "%d", presenter->getBrightness());

    //Setup the intro animation
    lightImageProgress.updateValue(0, 0);
    isAnimating = true;

    // Hide the big text areas to shorten the initial rendertime of the screen
    lightTextArea.setVisible(false);
    percentTextArea.setVisible(false);
}

void LightScreenView::tearDownScreen()
{
    LightScreenViewBase::tearDownScreen();
}

void LightScreenView::handleTickEvent()
{
    LightScreenViewBase::handleTickEvent();

    // Set the hidden text areas visible on first tick
    if (!lightTextArea.isVisible())
    {
        lightTextArea.setVisible(true);
        lightTextArea.invalidate();
        percentTextArea.setVisible(true);
        percentTextArea.invalidate();
    }
    else if (isAnimating)
    {
        // Create the animation by increasing the fill size until the actual value is reached
        int minValue, maxValue;
        lightImageProgress.getRange(minValue, maxValue);
        uint16_t currentProgress = lightImageProgress.getProgress(maxValue - minValue);
        if (currentProgress < presenter->getBrightness())
        {
            lightImageProgress.updateValue(currentProgress + 5, 0);
        }
        else
        {
            isAnimating = false;
        }
    }
}

void LightScreenView::increaseBrightness()
{
    changeBrightness(MAX_VALUE / NUMBER_OF_STEPS);
}

void LightScreenView::decreaseBrightness()
{
    changeBrightness(-MAX_VALUE / NUMBER_OF_STEPS);
}

void LightScreenView::changeBrightness(int change)
{
    const int newValue = presenter->getBrightness() + change;
    int minValue, maxValue;
    lightImageProgress.getRange(minValue, maxValue);

    if (newValue >= minValue && newValue <= maxValue)
    {
        presenter->setBrightness(newValue);

        // Update the number text
        Unicode::snprintf(lightTextAreaBuffer, LIGHTTEXTAREA_SIZE, "%d", newValue);
        lightTextArea.invalidate();

        // Only update the graphics after the intro initial animation is done
        if (!isAnimating)
        {
            lightImageProgress.updateValue(newValue, 0);
        }
    }
}
