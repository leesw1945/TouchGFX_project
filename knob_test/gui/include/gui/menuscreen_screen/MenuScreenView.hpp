#ifndef MENUSCREENVIEW_HPP
#define MENUSCREENVIEW_HPP

#include <gui_generated/menuscreen_screen/MenuScreenViewBase.hpp>
#include <gui/menuscreen_screen/MenuScreenPresenter.hpp>

class MenuScreenView : public MenuScreenViewBase
{
public:
    MenuScreenView();
    virtual ~MenuScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void menuScrollWheelUpdateItem(MenuIconCustomContainer& item, int16_t itemIndex);

    virtual void scrollDown();
    virtual void scrollUp();
    virtual void enter();
    virtual void handleTickEvent();
    virtual void handleClickEvent(const ClickEvent& event);

protected:
    void rollMenu(int16_t change);
};

#endif // MENUSCREENVIEW_HPP
