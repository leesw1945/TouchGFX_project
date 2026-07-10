#include <gui/batteryscreen_screen/BatteryScreenView.hpp>

BatteryScreenView::BatteryScreenView()
{

}

void BatteryScreenView::setupScreen()
{
    BatteryScreenViewBase::setupScreen();

    presenter->setAmbientLightRGB(0, 255, 0);

    updateBatteryIndicator();

    // Hide some widget to shorten the initial screen render time
    chargeIconImage.setVisible(false);
    chargeTextArea.setVisible(false);
    percentTextArea.setVisible(false);
}

void BatteryScreenView::tearDownScreen()
{
    BatteryScreenViewBase::tearDownScreen();
}

void BatteryScreenView::handleTickEvent()
{
    BatteryScreenViewBase::handleTickEvent();

    // Set the hidden widgets visible on the first tick
    if (!chargeTextArea.isVisible())
    {
        chargeIconImage.setVisible(true);
        chargeIconImage.invalidate();
        chargeTextArea.setVisible(true);
        chargeTextArea.invalidate();
        percentTextArea.setVisible(true);
        percentTextArea.invalidate();
    }

    updateBatteryIndicator();
}

void BatteryScreenView::updateBatteryIndicator()
{
    const uint16_t newBatteryLevel = presenter->getBatteryLevel();

    // Update the graphics to reflect the current battery level
    const int16_t newBatteryFillLevel = FILL_AREA_SIZE - (newBatteryLevel * FILL_AREA_SIZE) / 1000;
    if (newBatteryFillLevel != oldBatteryFillLevel)
    {
        waveAnimatedImage.invalidate();
        waveAnimatedImage.setY(newBatteryFillLevel);
        backgroundTiledImage.setHeight(newBatteryFillLevel);
        waveAnimatedImage.invalidate();

        oldBatteryFillLevel = newBatteryLevel;
    }

    // Update the text field to reflect the current battery level
    const uint16_t newBatteryPercentage = (newBatteryLevel * 100) / 1000;
    if (newBatteryPercentage != oldBatteryPercentage)
    {
        Unicode::snprintf(chargeTextAreaBuffer, CHARGETEXTAREA_SIZE, "%d", newBatteryPercentage);
        chargeTextArea.invalidate();

        oldBatteryPercentage = newBatteryPercentage;
    }
}
