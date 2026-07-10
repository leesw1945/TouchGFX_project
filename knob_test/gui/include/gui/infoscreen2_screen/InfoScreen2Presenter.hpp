#ifndef INFOSCREEN2PRESENTER_HPP
#define INFOSCREEN2PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class InfoScreen2View;

class InfoScreen2Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    InfoScreen2Presenter(InfoScreen2View& v);

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

    virtual ~InfoScreen2Presenter() {}

private:
    InfoScreen2Presenter();

    InfoScreen2View& view;
};

#endif // INFOSCREEN2PRESENTER_HPP
