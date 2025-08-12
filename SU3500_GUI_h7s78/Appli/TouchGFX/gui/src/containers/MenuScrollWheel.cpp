#include <gui/containers/MenuScrollWheel.hpp>
#include <images/BitmapDatabase.hpp>

MenuScrollWheel::MenuScrollWheel()
{

}

void MenuScrollWheel::initialize()
{
    MenuScrollWheelBase::initialize();
}


// 일반 스크롤 아이템 아이콘 업데이트 (현재 사용 가능한 ID로 수정)
void MenuScrollWheel::scrollWheel1UpdateItem(MenuScrollBtn& item, int16_t itemIndex)
{
    switch (itemIndex % 5)  // 5개 아이콘을 순환
    {
    case 0:
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_MAPS_LOCAL_HOSPITAL_70_70_000000_SVG_ID));
        break;
    case 1:
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_ACTION_BOOKMARKS_70_70_000000_SVG_ID));
        break;
    case 2:
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_ACTION_BUILD_CIRCLE_70_70_000000_SVG_ID));
        break;
    case 3:
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_ALERT_ERROR_OUTLINE_70_70_000000_SVG_ID));
        break;
    case 4:
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_ACTION_SETTINGS_70_70_000000_SVG_ID));
        break;
    default:
        // 기본 아이콘 (첫 번째 아이콘)
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_MAPS_LOCAL_HOSPITAL_70_70_000000_SVG_ID));
        break;
    }
}

// 중앙(선택된) 스크롤 아이템 아이콘 업데이트 (현재 사용 가능한 ID로 수정)
void MenuScrollWheel::scrollWheel1UpdateCenterItem(MenuScrollCenterBtn& item, int16_t itemIndex)
{
    switch (itemIndex % 5)  // 5개 아이콘을 순환
    {
    case 0:
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_MAPS_LOCAL_HOSPITAL_90_90_000000_SVG_ID));
        break;
    case 1:
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_ACTION_BOOKMARKS_90_90_000000_SVG_ID));
        break;
    case 2:
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_ACTION_BUILD_CIRCLE_90_90_000000_SVG_ID));
        break;
    case 3:
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_ALERT_ERROR_OUTLINE_90_90_000000_SVG_ID));
        break;
    case 4:
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_ACTION_SETTINGS_90_90_000000_SVG_ID));
        break;
    default:
        // 기본 아이콘 (첫 번째 아이콘)
        item.updateIcon(touchgfx::Bitmap(BITMAP_ICON_THEME_IMAGES_MAPS_LOCAL_HOSPITAL_90_90_000000_SVG_ID));
        break;
    }
}