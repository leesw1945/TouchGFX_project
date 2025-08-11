#include <gui/containers/MenuScrollCenterBtn.hpp>

MenuScrollCenterBtn::MenuScrollCenterBtn()
{

}

void MenuScrollCenterBtn::initialize()
{
    MenuScrollCenterBtnBase::initialize();
}

void MenuScrollCenterBtn::updateIcon(touchgfx::Bitmap icon)
{
    buttonWithIcon1.setIconBitmap(icon);
    buttonWithIcon1.invalidate();
}
