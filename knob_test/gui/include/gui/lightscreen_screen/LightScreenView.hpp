#ifndef LIGHTSCREENVIEW_HPP
#define LIGHTSCREENVIEW_HPP

#include <gui_generated/lightscreen_screen/LightScreenViewBase.hpp>
#include <gui/lightscreen_screen/LightScreenPresenter.hpp>

class LightScreenView : public LightScreenViewBase
{
public:
    LightScreenView();
    virtual ~LightScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

    virtual void increaseBrightness();
    virtual void decreaseBrightness();

protected:
    const uint16_t FULL_CIRCLE = 360;
    const uint16_t NUMBER_OF_STEPS = 20;
    const uint16_t ANIMATION_SPEED = 18;
    const uint16_t MIN_VALUE = 0;
    const uint16_t MAX_VALUE = 100;

    void changeBrightness(int change);

    bool isAnimating = false;
};

#endif // LIGHTSCREENVIEW_HPP
