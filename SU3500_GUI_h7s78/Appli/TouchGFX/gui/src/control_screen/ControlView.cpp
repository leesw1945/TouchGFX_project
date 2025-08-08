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

void ControlView::moveMenuScroll()
{

	int currentY = menuScrollWheel1.getY();

	menuScrollWheel1.clearMoveAnimationEndedAction();

	if (currentY < 0) {

		menuScrollWheel1.startMoveAnimation(0, 0, 30,
				touchgfx::EasingEquations::cubicEaseOut,
				touchgfx::EasingEquations::cubicEaseOut);

	} else {

		menuScrollWheel1.startMoveAnimation(0, -480, 30,
				touchgfx::EasingEquations::cubicEaseIn,
				touchgfx::EasingEquations::cubicEaseIn);

	}

}

