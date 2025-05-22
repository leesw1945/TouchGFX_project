#ifndef CLOCKVIEW_HPP
#define CLOCKVIEW_HPP

#include <gui_generated/clock_screen/ClockViewBase.hpp>
#include <gui/clock_screen/ClockPresenter.hpp>

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
};

#endif // CLOCKVIEW_HPP
