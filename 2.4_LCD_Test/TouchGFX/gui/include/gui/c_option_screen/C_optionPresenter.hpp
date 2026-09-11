#ifndef C_OPTIONPRESENTER_HPP
#define C_OPTIONPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class C_optionView;

class C_optionPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    C_optionPresenter(C_optionView& v);

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

    virtual ~C_optionPresenter() {}

private:
    C_optionPresenter();

    C_optionView& view;
};

#endif // C_OPTIONPRESENTER_HPP
