#ifndef SETTINGVIEW_HPP
#define SETTINGVIEW_HPP

#include <gui_generated/setting_screen/SettingViewBase.hpp>
#include <gui/setting_screen/SettingPresenter.hpp>

class SettingView : public SettingViewBase
{
public:
    SettingView();
    virtual ~SettingView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // SETTINGVIEW_HPP
