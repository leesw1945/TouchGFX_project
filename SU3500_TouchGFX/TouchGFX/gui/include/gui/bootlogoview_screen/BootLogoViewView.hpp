#ifndef BOOTLOGOVIEWVIEW_HPP
#define BOOTLOGOVIEWVIEW_HPP

#include <gui_generated/bootlogoview_screen/BootLogoViewViewBase.hpp>
#include <gui/bootlogoview_screen/BootLogoViewPresenter.hpp>

class BootLogoViewView : public BootLogoViewViewBase
{
public:
    BootLogoViewView();
    virtual ~BootLogoViewView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // BOOTLOGOVIEWVIEW_HPP
