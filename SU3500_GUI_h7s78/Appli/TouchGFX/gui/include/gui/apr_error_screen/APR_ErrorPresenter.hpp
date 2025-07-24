#ifndef APR_ERRORPRESENTER_HPP
#define APR_ERRORPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class APR_ErrorView;

class APR_ErrorPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    APR_ErrorPresenter(APR_ErrorView& v);

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

    virtual ~APR_ErrorPresenter() {}

private:
    APR_ErrorPresenter();

    APR_ErrorView& view;
};

#endif // APR_ERRORPRESENTER_HPP
