#include <gui/containers/MenuScrollCenterBtn.hpp>
#include <touchgfx\Bitmap.hpp>
#include <images/BitmapDatabase.hpp>

MenuScrollCenterBtn::MenuScrollCenterBtn()
{

}

void MenuScrollCenterBtn::initialize()
{
    MenuScrollCenterBtnBase::initialize();
}

void MenuScrollCenterBtn::updateIcon(touchgfx::Bitmap icon)
{

    touchgfx::Bitmap backgroundNormal(BITMAP_CLAY_THEME_IMAGES_WIDGETS_BUTTON_ICON_ROUNDED_LARGE_FILL_NORMAL_ID);
    touchgfx::Bitmap backgroundPressed(BITMAP_CLAY_THEME_IMAGES_WIDGETS_BUTTON_ICON_ROUNDED_LARGE_FILL_PRESSED_ID);
    buttonWithIcon1.setBitmaps(backgroundNormal, backgroundPressed, icon, icon);
    buttonWithIcon1.invalidate();
}
