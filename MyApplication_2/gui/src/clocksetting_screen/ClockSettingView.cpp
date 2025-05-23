#include <gui/clocksetting_screen/ClockSettingView.hpp>

ClockSettingView::ClockSettingView()
    : hour(0), minute(0)
{
    textAreaHourBuffer[0] = 0;
    textAreaMinuteBuffer[0] = 0;
}

void ClockSettingView::setupScreen()
{
    ClockSettingViewBase::setupScreen();

    hour = presenter->getHour();
    minute = presenter->getMinute();

    Unicode::snprintf(textAreaHourBuffer, TEXTAREAHOUR_SIZE, "%02d", hour);
    Unicode::snprintf(textAreaMinuteBuffer, TEXTAREAMINUTE_SIZE, "%02d", minute);

    textAreaHour.setWildcard(textAreaHourBuffer);
    textAreaMinute.setWildcard(textAreaMinuteBuffer);
}

void ClockSettingView::tearDownScreen()
{
    ClockSettingViewBase::tearDownScreen();
}

void ClockSettingView::buttonHourUpClicked()
{
    hour = (hour + 1) % 24; // Keep new value in range 0..23
    Unicode::snprintf(textAreaHourBuffer, TEXTAREAHOUR_SIZE, "%02d", hour);
    textAreaHour.setWildcard(textAreaHourBuffer);
    textAreaHour.invalidate();
}

void ClockSettingView::buttonHourDownClicked()
{
    hour = (hour + 24 - 1) % 24; // Keep new value in range 0..23
    Unicode::snprintf(textAreaHourBuffer, TEXTAREAHOUR_SIZE, "%02d", hour);
    textAreaHour.setWildcard(textAreaHourBuffer);
    textAreaHour.invalidate();
}

void ClockSettingView::buttonMinuteUpClicked()
{
    minute = (minute + 1) % 60; // Keep new value in range 0..59
    Unicode::snprintf(textAreaMinuteBuffer, TEXTAREAMINUTE_SIZE, "%02d", minute);
    textAreaMinute.setWildcard(textAreaMinuteBuffer);
    textAreaMinute.invalidate();
}

void ClockSettingView::buttonMinuteDownClicked()
{
    minute = (minute + 60 - 1) % 60; // Keep new value in range 0..59
    Unicode::snprintf(textAreaMinuteBuffer, TEXTAREAMINUTE_SIZE, "%02d", minute);
    textAreaMinute.setWildcard(textAreaMinuteBuffer);
    textAreaMinute.invalidate();
}