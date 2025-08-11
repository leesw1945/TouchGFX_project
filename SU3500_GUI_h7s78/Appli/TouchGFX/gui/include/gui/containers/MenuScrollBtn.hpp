#ifndef MENUSCROLLBTN_HPP
#define MENUSCROLLBTN_HPP

#include <gui_generated/containers/MenuScrollBtnBase.hpp>
#include <touchgfx/Bitmap.hpp>

class MenuScrollBtn : public MenuScrollBtnBase
{
public:
    MenuScrollBtn();
    virtual ~MenuScrollBtn() {}

    virtual void initialize();

    void updateIcon(touchgfx::Bitmap icon);
    
protected:
};

#endif // MENUSCROLLBTN_HPP
