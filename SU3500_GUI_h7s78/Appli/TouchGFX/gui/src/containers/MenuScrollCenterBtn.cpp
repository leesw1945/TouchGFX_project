#include <gui/containers/MenuScrollCenterBtn.hpp>
#include <gui/containers/MenuScrollWheel.hpp>
#include <touchgfx\Bitmap.hpp>
#include <images/BitmapDatabase.hpp>

MenuScrollCenterBtn::MenuScrollCenterBtn() : 
    buttonCallback(this, &MenuScrollCenterBtn::buttonCallbackHandler),
    parentWheel(nullptr)
{

}

void MenuScrollCenterBtn::initialize()
{
    MenuScrollCenterBtnBase::initialize();

    // 버튼 클릭 콜백 설정
    buttonWithIcon1.setAction(buttonCallback);
}

void MenuScrollCenterBtn::updateIcon(touchgfx::Bitmap normal, touchgfx::Bitmap pressed)
{
    touchgfx::Bitmap backgroundNormal(BITMAP_CLAY_THEME_IMAGES_WIDGETS_BUTTON_ICON_ROUNDED_LARGE_FILL_NORMAL_ID);
    touchgfx::Bitmap backgroundPressed(BITMAP_CLAY_THEME_IMAGES_WIDGETS_BUTTON_ICON_ROUNDED_LARGE_FILL_PRESSED_ID);
    buttonWithIcon1.setBitmaps(backgroundNormal, backgroundPressed, normal, pressed);
    buttonWithIcon1.invalidate();
}

void MenuScrollCenterBtn::setCenterButtonAction(MenuScrollWheel* parent)
{
    parentWheel = parent;
}

void MenuScrollCenterBtn::buttonCallbackHandler(const touchgfx::AbstractButton& src)
{
    if (&src == &buttonWithIcon1 && parentWheel != nullptr)
    {
        // 부모 MenuScrollWheel의 centerButtonClicked 메서드 호출
        parentWheel->centerButtonClicked();
    }
}