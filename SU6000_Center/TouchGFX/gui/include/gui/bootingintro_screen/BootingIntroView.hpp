#ifndef BOOTINGINTROVIEW_HPP
#define BOOTINGINTROVIEW_HPP

#include <gui_generated/bootingintro_screen/BootingIntroViewBase.hpp>
#include <gui/bootingintro_screen/BootingIntroPresenter.hpp>

class BootingIntroView : public BootingIntroViewBase
{
public:
    BootingIntroView();
    virtual ~BootingIntroView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // BOOTINGINTROVIEW_HPP
