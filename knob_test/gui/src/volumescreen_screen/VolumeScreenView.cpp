#include <gui/volumescreen_screen/VolumeScreenView.hpp>

VolumeScreenView::VolumeScreenView()
{

}

void VolumeScreenView::setupScreen()
{
    VolumeScreenViewBase::setupScreen();

    presenter->setAmbientLightRGB(0, 0, 255);

    Unicode::snprintf(volumeTextAreaBuffer, VOLUMETEXTAREA_SIZE, "%d", presenter->getVolume());

    // Setup the intro animation
    circleProgress.updateValue(0, 0);
    isAnimating = true;

    // Hide the bug text area to shorten the initial screen rendertime
    volumeTextArea.setVisible(false);
}

void VolumeScreenView::tearDownScreen()
{
    VolumeScreenViewBase::tearDownScreen();
}

void VolumeScreenView::handleTickEvent()
{
    VolumeScreenViewBase::handleTickEvent();

    // Set the big number text area visble on first tick
    if (!volumeTextArea.isVisible())
    {
        volumeTextArea.setVisible(true);
        volumeTextArea.invalidate();
    }

    // Create the intro animation by increasing the displayed progress until it reaches the actual value
    else if (isAnimating)
    {
        int minValue, maxValue;
        circleProgress.getRange(minValue, maxValue);
        const uint16_t currentProgress = circleProgress.getProgress(maxValue - minValue);
        if (currentProgress < presenter->getVolume())
        {
            circleProgress.updateValue(currentProgress + 1, 0);
        }
        else
        {
            isAnimating = false;
        }
    }
}

void VolumeScreenView::increaseVolume()
{
    changeVolume(1);
}

void VolumeScreenView::decreaseVolume()
{
    changeVolume(-1);
}

void VolumeScreenView::changeVolume(int change)
{
    const int newValue = presenter->getVolume() + change;
    int minValue, maxValue;
    circleProgress.getRange(minValue, maxValue);

    if (newValue >= minValue && newValue <= maxValue)
    {
        presenter->setVolume(newValue);

        // Update the number in the big text field
        Unicode::snprintf(volumeTextAreaBuffer, VOLUMETEXTAREA_SIZE, "%d", newValue);
        volumeTextArea.invalidate();

        if (!isAnimating)
        {
            circleProgress.updateValue(newValue, 0);
        }
    }
}
