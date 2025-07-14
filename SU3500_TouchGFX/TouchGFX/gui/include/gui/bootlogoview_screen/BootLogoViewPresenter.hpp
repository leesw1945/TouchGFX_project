#ifndef BOOTLOGOVIEWPRESENTER_HPP
#define BOOTLOGOVIEWPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class BootLogoViewView;

class BootLogoViewPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    BootLogoViewPresenter(BootLogoViewView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~BootLogoViewPresenter() {}

private:
    BootLogoViewPresenter();

    BootLogoViewView& view;
};

#endif // BOOTLOGOVIEWPRESENTER_HPP
