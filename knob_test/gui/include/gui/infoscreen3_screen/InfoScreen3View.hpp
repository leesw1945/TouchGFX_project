#ifndef INFOSCREEN3VIEW_HPP
#define INFOSCREEN3VIEW_HPP

#include <gui_generated/infoscreen3_screen/InfoScreen3ViewBase.hpp>
#include <gui/infoscreen3_screen/InfoScreen3Presenter.hpp>

class InfoScreen3View : public InfoScreen3ViewBase
{
public:
    InfoScreen3View();
    virtual ~InfoScreen3View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // INFOSCREEN3VIEW_HPP
