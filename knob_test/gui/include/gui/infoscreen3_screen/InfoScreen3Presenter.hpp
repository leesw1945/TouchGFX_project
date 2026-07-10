#ifndef INFOSCREEN3PRESENTER_HPP
#define INFOSCREEN3PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class InfoScreen3View;

class InfoScreen3Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    InfoScreen3Presenter(InfoScreen3View& v);

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

    virtual ~InfoScreen3Presenter() {}

private:
    InfoScreen3Presenter();

    InfoScreen3View& view;
};

#endif // INFOSCREEN3PRESENTER_HPP
