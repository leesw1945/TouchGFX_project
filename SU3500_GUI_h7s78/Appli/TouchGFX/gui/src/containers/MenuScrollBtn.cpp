#include <gui/containers/MenuScrollBtn.hpp>
#include <touchgfx\Bitmap.hpp>
#include <images/BitmapDatabase.hpp>

MenuScrollBtn::MenuScrollBtn()
{

}

void MenuScrollBtn::initialize()
{
    MenuScrollBtnBase::initialize();
}

void MenuScrollBtn::updateIcon(touchgfx::Bitmap icon)
{

    //touchgfx::Bitmap backgroundNormal(BITMAP_CLAY_THEME_IMAGES_WIDGETS_BUTTON_ICON_ROUNDED_MEDIUM_FILL_NORMAL_ID);
    //touchgfx::Bitmap backgroundPressed = backgroundNormal;
    touchgfx::Bitmap background(BITMAP_CLAY_THEME_IMAGES_WIDGETS_BUTTON_ICON_ROUNDED_MEDIUM_FILL_NORMAL_ID);

    //(BITMAP_CLAY_THEME_IMAGES_WIDGETS_BUTTON_ICON_ROUNDED_MEDIUM_FILL_PRESSED_ID);
    buttonWithIcon1.setBitmaps(background, background, icon, icon);
    buttonWithIcon1.invalidate();
}