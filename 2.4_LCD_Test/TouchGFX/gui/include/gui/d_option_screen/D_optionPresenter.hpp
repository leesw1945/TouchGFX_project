#ifndef D_OPTIONPRESENTER_HPP
#define D_OPTIONPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class D_optionView;

class D_optionPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    D_optionPresenter(D_optionView& v);

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

    virtual ~D_optionPresenter() {}

private:
    D_optionPresenter();

    D_optionView& view;
};

#endif // D_OPTIONPRESENTER_HPP
