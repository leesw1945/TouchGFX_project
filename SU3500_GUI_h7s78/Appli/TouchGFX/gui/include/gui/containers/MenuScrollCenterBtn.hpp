#ifndef MENUSCROLLCENTERBTN_HPP
#define MENUSCROLLCENTERBTN_HPP

#include <gui_generated/containers/MenuScrollCenterBtnBase.hpp>
#include <touchgfx/Bitmap.hpp>

class MenuScrollCenterBtn : public MenuScrollCenterBtnBase
{
public:
    MenuScrollCenterBtn();
    virtual ~MenuScrollCenterBtn() {}

    virtual void initialize();

    void updateIcon(touchgfx::Bitmap icon);
    
protected:
};

#endif // MENUSCROLLCENTERBTN_HPP
