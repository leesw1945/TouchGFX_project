#include <gui/clock_screen/ClockView.hpp>

ClockView::ClockView()
    : hour(0), minute(0), tickCount(0), addStart(1), addEnd(2)
{
    textClockBuffer1[0] = 0;
    textClockBuffer2[0] = 0;
}

void ClockView::setupScreen()
{
    ClockViewBase::setupScreen();

    hour = presenter->getHour();
    minute = presenter->getMinute();

    Unicode::snprintf(textClockBuffer1, TEXTCLOCKBUFFER1_SIZE, "%02d", hour);
    Unicode::snprintf(textClockBuffer2, TEXTCLOCKBUFFER2_SIZE, "%02d", minute);

    textClock.setWildcard1(textClockBuffer1);
    textClock.setWildcard2(textClockBuffer2);
}

void ClockView::tearDownScreen()
{
    presenter->saveHour(hour);
    presenter->saveMinute(minute);
    
    ClockViewBase::tearDownScreen();
}

void ClockView::handleTickEvent()
{
    if (tickCount == 60)
    {
        minute++;
        hour = (hour + (minute / 60)) % 24;
        minute %= 60;

        Unicode::snprintf(textClockBuffer1, TEXTCLOCKBUFFER1_SIZE, "%02d", hour);
        Unicode::snprintf(textClockBuffer2, TEXTCLOCKBUFFER2_SIZE, "%02d", minute);

        textClock.invalidate();

        tickCount = 0;
    }

    if (!textClock.isMoveAnimationRunning())
    {
        tickCount++;
        if (circle.getArcStart() + 340 == circle.getArcEnd())
        {
            addStart = 2;
            addEnd = 1;
        }
        else if (circle.getArcStart() + 20 == circle.getArcEnd())
        {
            addStart = 1;
            addEnd = 2;
        }
        if (circle.getArcStart() > 360 && circle.getArcEnd() > 360)
        {
            circle.updateArc(circle.getArcStart() - 360, circle.getArcEnd() - 360);
        }
        circle.updateArc(circle.getArcStart() + addStart, circle.getArcEnd() + addEnd);
    }
}