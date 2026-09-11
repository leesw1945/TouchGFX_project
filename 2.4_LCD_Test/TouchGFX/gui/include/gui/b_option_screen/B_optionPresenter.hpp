#ifndef B_OPTIONPRESENTER_HPP
#define B_OPTIONPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class B_optionView;

class B_optionPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    B_optionPresenter(B_optionView& v);

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

    virtual ~B_optionPresenter() {}

private:
    B_optionPresenter();

    B_optionView& view;
};

#endif // B_OPTIONPRESENTER_HPP
