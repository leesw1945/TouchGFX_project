#ifndef MENUSCROLLCENTERBTN_HPP
#define MENUSCROLLCENTERBTN_HPP

#include <gui_generated/containers/MenuScrollCenterBtnBase.hpp>
#include <touchgfx/Bitmap.hpp>

class MenuScrollWheel;

class MenuScrollCenterBtn : public MenuScrollCenterBtnBase
{
public:
    MenuScrollCenterBtn();
    virtual ~MenuScrollCenterBtn() {}

    virtual void initialize();

    void updateIcon(touchgfx::Bitmap icon);

    // 클릭 액션 설정을 위한 메서드
    void setCenterButtonAction(MenuScrollWheel* parent);
    
protected:
    // 버튼 클릭 콜백
    touchgfx::Callback<MenuScrollCenterBtn, const touchgfx::AbstractButton&> buttonCallback;
    void buttonCallbackHandler(const touchgfx::AbstractButton& src);
    
    // 부모 MenuScrollWheel 참조
    MenuScrollWheel* parentWheel;
};

#endif // MENUSCROLLCENTERBTN_HPP
