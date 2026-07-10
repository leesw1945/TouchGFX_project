#include <gui/gaugescreen_screen/GaugeScreenView.hpp>

GaugeScreenView::GaugeScreenView()
{

}

void GaugeScreenView::setupScreen()
{
    GaugeScreenViewBase::setupScreen();

    presenter->setAmbientLightRGB(255, 0, 255);

    Unicode::snprintf(valueTextAreaBuffer, VALUETEXTAREA_SIZE, "%d", presenter->getPressure());

    //Setup the intro animation
    isAnimating = true;
    moveNeedleToPostion(0);
    valueTextArea.setVisible(false);
}

void GaugeScreenView::tearDownScreen()
{
    GaugeScreenViewBase::tearDownScreen();
}

void GaugeScreenView::handleTickEvent()
{
    GaugeScreenViewBase::handleTickEvent();

    tickCount++;

    // Set the big number text area visible on the first tick
    if (!valueTextArea.isVisible())
    {
        valueTextArea.setVisible(true);
        valueTextArea.invalidate();
    }

    // Create the intro animation by moving the needle image one position forward until it reaches the actual value
    else if (isAnimating && tickCount % 2 == 1)
    {
        uint16_t currentNeedlePostion = needleImage.getBitmapId() - BITMAP_GAUGE_NEEDLE_00_ID;
        if (currentNeedlePostion < presenter->getPressure())
        {
            moveNeedleToPostion(currentNeedlePostion + 1);
        }
        else
        {
            isAnimating = false;
        }
    }
}

void GaugeScreenView::increasePressure()
{
    changeValue(1);
}

void GaugeScreenView::decreasePressure()
{
    changeValue(-1);
}

void GaugeScreenView::changeValue(const int16_t change)
{
    // Calculate the new value and check if it is in the valid range
    const int16_t newValue = presenter->getPressure() + change;
    if (newValue >= 0 && newValue < NUMBER_OF_POSITIONS)
    {
        presenter->setPressure(newValue);

        // Update the text field
        Unicode::snprintf(valueTextAreaBuffer, VALUETEXTAREA_SIZE, "%d", newValue);
        valueTextArea.invalidate();

        if (!isAnimating)
        {
            moveNeedleToPostion(newValue);
        }
    }
}

void GaugeScreenView::moveNeedleToPostion(uint16_t position)
{
    if (position < NUMBER_OF_POSITIONS)
    {
        // The gauge needle is implemented as a simple image widget
        needleImage.invalidate();
        needleImage.setX(needlePositions[position].x);
        needleImage.setY(needlePositions[position].y);
        needleImage.setBitmap(needlePositions[position].bitmapId);
        needleImage.invalidate();
    }
}
