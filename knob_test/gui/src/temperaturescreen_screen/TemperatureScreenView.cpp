#include <gui/temperaturescreen_screen/TemperatureScreenView.hpp>

TemperatureScreenView::TemperatureScreenView()
{

}

void TemperatureScreenView::setupScreen()
{
    TemperatureScreenViewBase::setupScreen();

    // Update the number in the big text field(s)
    int16_t currentTemperautre = presenter->getTemperature();
    Unicode::snprintf(temperatureTextAreaBuffer, TEMPERATURETEXTAREA_SIZE, "%d", currentTemperautre);
    Unicode::snprintf(alternativeTemperatureTextAreaBuffer, ALTERNATIVETEMPERATURETEXTAREA_SIZE, "%d", currentTemperautre);

    presenter->setAmbientLightRGB(255, 0, 0);

    //Setup the intro animation
    fillContainer.setHeight(FILL_AREA_SIZE);
    isAnimating = true;

    // Hide the big number text field to shorten the initial screen rendertime
    temperatureTextArea.setVisible(false);
}

void TemperatureScreenView::tearDownScreen()
{
    TemperatureScreenViewBase::tearDownScreen();
}

void TemperatureScreenView::handleTickEvent()
{
    TemperatureScreenViewBase::handleTickEvent();

    // Set the number text field visible on the first tick
    if (!temperatureTextArea.isVisible())
    {
        temperatureTextArea.setVisible(true);
        temperatureTextArea.invalidate();
    }

    // Create the intro animation by gradually increasing the fill area until the actual level is reached
    else if (isAnimating)
    {
        if (fillContainer.getHeight() > FILL_AREA_SIZE - (presenter->getTemperature() - MIN_VALUE) * LINES_PER_DEGREE)
        {
            updateFill(fillContainer.getHeight() - 8);
        }
        else
        {
            isAnimating = false;
        }
    }
}

void TemperatureScreenView::increaseTemperature()
{
    changeTemperature(1);
}

void TemperatureScreenView::decreaseTemperature()
{
    changeTemperature(-1);
}

void TemperatureScreenView::changeTemperature(const int16_t change)
{
    int16_t newTemperature = presenter->getTemperature() + change;
    if (newTemperature >= MIN_VALUE && newTemperature <= MAX_VALUE)
    {
        presenter->setTemperature(newTemperature);

        // Update the value in the big text field
        Unicode::snprintf(temperatureTextAreaBuffer, TEMPERATURETEXTAREA_SIZE, "%d", newTemperature);
        Unicode::snprintf(alternativeTemperatureTextAreaBuffer, ALTERNATIVETEMPERATURETEXTAREA_SIZE, "%d", newTemperature);
        alternativeTemperatureTextArea.invalidate();

        if (!isAnimating)
        {
            const int16_t newLevel = FILL_AREA_SIZE - (newTemperature - MIN_VALUE) * LINES_PER_DEGREE;
            updateFill(newLevel);
        }
    }
}

void TemperatureScreenView::updateFill(const int16_t newLevel)
{
    // Change the container height, to change the fill level
    const int16_t oldLevel = fillContainer.getHeight();
    fillContainer.setHeight(newLevel);

    // Invalidate the screen area which was changed
    Rect updatedFill;
    updatedFill.x = 0;
    updatedFill.width = FILL_AREA_SIZE;
    if (newLevel > oldLevel)
    {
        updatedFill.y = oldLevel;
        updatedFill.height = newLevel - oldLevel;
    }
    else
    {
        updatedFill.y = newLevel;
        updatedFill.height = oldLevel - newLevel;
    }
    invalidateRect(updatedFill);
}
