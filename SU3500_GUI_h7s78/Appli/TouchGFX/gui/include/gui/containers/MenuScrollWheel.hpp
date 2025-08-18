#ifndef MENUSCROLLWHEEL_HPP
#define MENUSCROLLWHEEL_HPP

#include <gui_generated/containers/MenuScrollWheelBase.hpp>

class MenuScrollWheel : public MenuScrollWheelBase
{
public:
    MenuScrollWheel();
    virtual ~MenuScrollWheel() {}

    virtual void initialize();

    //virtual void scrollWheel1UpdateItem(MenuScrollBtn& item, int16_t itemIndex) override;
    virtual void scrollWheel1UpdateCenterItem(MenuScrollCenterBtn& item, int16_t itemIndex) override;
    virtual void scrollWheel1UpdateItem(MenuScrollImage& item, int16_t itemIndex) override;

    // 버튼 클릭 처리를 위한 메서드 추가
    void centerButtonClicked();
    
    
protected:
};

#endif // MENUSCROLLWHEEL_HPP
