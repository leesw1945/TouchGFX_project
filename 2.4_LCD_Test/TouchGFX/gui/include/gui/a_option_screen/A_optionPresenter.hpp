#ifndef A_OPTIONPRESENTER_HPP
#define A_OPTIONPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class A_optionView;

class A_optionPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    A_optionPresenter(A_optionView& v);

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

    virtual ~A_optionPresenter() {}

private:
    A_optionPresenter();

    A_optionView& view;
};

#endif // A_OPTIONPRESENTER_HPP
