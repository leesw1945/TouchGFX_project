#include <gui/control_screen/ControlView.hpp>
#include <touchgfx/Color.hpp>
#include <stdlib.h>

ControlView::ControlView()
{

}

void ControlView::setupScreen()
{
    ControlViewBase::setupScreen();
}

void ControlView::tearDownScreen()
{
    ControlViewBase::tearDownScreen();
}

void ControlView::changeColor()
{
	box1.setColor(touchgfx::Color::getColorFromRGB(rand()&0xff, rand()&0xff, rand()&0xff));
	box1.invalidate();
}
