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
    
    item.setCenterButtonAction(this);

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

// 중앙 버튼 클릭 시 호출되는 메서드
void MenuScrollWheel::centerButtonClicked()
{

    int16_t currentIndex = scrollWheel1.getSelectedItem();

    switch (currentIndex % 5)
    {
    case 0:
        // 컨트롤 화면 전환
        application().gotoControlScreenWipeTransitionWest();
        break;
    case 1:
        // APR 화면 전환
        application().gotoAutoPostionSetScreenWipeTransitionEast();
        break;
    case 2:
        // 캘리브레이션 화면 전환
        application().gotoCalibrationScreenWipeTransitionEast();
        break;
    case 3:
        // 에러 화면 전환
        application().gotoApr_ErrorScreenWipeTransitionEast();
        break;
    case 4:
        // 설정 화면 전환
        application().gotoSettingScreenWipeTransitionEast();
        break;
    default:
        // 첫 번째 화면 전환
        application().gotoControlScreenWipeTransitionWest();
        break;
    }
}