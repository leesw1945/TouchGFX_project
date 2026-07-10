#ifndef INFOSCREEN1PRESENTER_HPP
#define INFOSCREEN1PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class InfoScreen1View;

class InfoScreen1Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    InfoScreen1Presenter(InfoScreen1View& v);

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

    virtual ~InfoScreen1Presenter() {}

private:
    InfoScreen1Presenter();

    InfoScreen1View& view;
};

#endif // INFOSCREEN1PRESENTER_HPP
