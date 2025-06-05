#ifndef COMPASSSCREENVIEW_HPP
#define COMPASSSCREENVIEW_HPP

#include <gui_generated/compassscreen_screen/CompassScreenViewBase.hpp>
#include <gui/compassscreen_screen/CompassScreenPresenter.hpp>

struct Wheel
{
    bool update(float dt); //!< returns \c true if UI requires an update

    float _angle_rad = 0.0f;
    float _velocity = 0.0f;
    bool _dirty = false;
};

class CompassScreenView : public CompassScreenViewBase
{
public:
    CompassScreenView();
    virtual ~CompassScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
    void handleDragEvent(const touchgfx::DragEvent& event);
    void handleGestureEvent(const touchgfx::GestureEvent& event);
    void handleTickEvent() override;
    void updateView();

private:
    int32_t computeRangeIndex(float angle_deg);
    Wheel _wheel;
    uint32_t _tick = 0;
};

#endif // COMPASSSCREENVIEW_HPP
