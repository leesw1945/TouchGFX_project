#ifndef MENUSCROLLWHEEL_HPP
#define MENUSCROLLWHEEL_HPP

#include <gui_generated/containers/MenuScrollWheelBase.hpp>

class MenuScrollWheel : public MenuScrollWheelBase
{
public:
    MenuScrollWheel();
    virtual ~MenuScrollWheel() {}

    virtual void initialize();

    virtual void scrollWheel1UpdateItem(MenuScrollBtn& item, int16_t itemIndex) override;
    virtual void scrollWheel1UpdateCenterItem(MenuScrollCenterBtn& item, int16_t itemIndex) override;
    
protected:
};

#endif // MENUSCROLLWHEEL_HPP
