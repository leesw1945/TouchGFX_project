#ifndef CLOCKSETTINGVIEW_HPP
#define CLOCKSETTINGVIEW_HPP

#include <gui_generated/clocksetting_screen/ClockSettingViewBase.hpp>
#include <gui/clocksetting_screen/ClockSettingPresenter.hpp>
#include <touchgfx/Unicode.hpp>

class ClockSettingView : public ClockSettingViewBase
{
public:
    ClockSettingView();
    virtual ~ClockSettingView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void buttonHourUpClicked();
    virtual void buttonHourDownClicked();
    virtual void buttonMinuteUpClicked();
    virtual void buttonMinuteDownClicked();

    virtual void buttonSaveHourClicked()
    {
        presenter->saveHour(hour);
    }

    virtual void buttonSaveMinuteClicked()
    {
        presenter->saveMinute(minute);
    }
protected:
    int16_t hour;
    int16_t minute;

    static const uint16_t TEXTAREAHOUR_SIZE = 16;
    static const uint16_t TEXTAREAMINUTE_SIZE = 16;

    touchgfx::Unicode::UnicodeChar textAreaHourBuffer[TEXTAREAHOUR_SIZE];
    touchgfx::Unicode::UnicodeChar textAreaMinuteBuffer[TEXTAREAMINUTE_SIZE];
};

#endif // CLOCKSETTINGVIEW_HPP
