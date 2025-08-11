#include <gui/containers/MenuScrollBtn.hpp>

MenuScrollBtn::MenuScrollBtn()
{

}

void MenuScrollBtn::initialize()
{
    MenuScrollBtnBase::initialize();
}

void MenuScrollBtn::updateIcon(touchgfx::Bitmap icon)
{
    buttonWithIcon1.setIconBitmap(icon);
    buttonWithIcon1.invalidate();
}