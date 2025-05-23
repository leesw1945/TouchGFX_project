#ifndef CLOCKVIEW_HPP
#define CLOCKVIEW_HPP

#include <gui_generated/clock_screen/ClockViewBase.hpp>
#include <gui/clock_screen/ClockPresenter.hpp>
#include <touchgfx/Unicode.hpp>

class ClockView : public ClockViewBase
{
public:
    ClockView();
    virtual ~ClockView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
protected:
    int16_t hour;
    int16_t minute;
    int16_t tickCount;
    int16_t addStart;
    int16_t addEnd;

    static const uint16_t TEXTCLOCKBUFFER1_SIZE = 16;
    static const uint16_t TEXTCLOCKBUFFER2_SIZE = 16;
    
    touchgfx::Unicode::UnicodeChar textClockBuffer1[TEXTCLOCKBUFFER1_SIZE];
    touchgfx::Unicode::UnicodeChar textClockBuffer2[TEXTCLOCKBUFFER2_SIZE];
};

#endif // CLOCKVIEW_HPP
