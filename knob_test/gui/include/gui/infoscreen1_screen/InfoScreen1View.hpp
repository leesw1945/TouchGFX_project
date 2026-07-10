#ifndef INFOSCREEN1VIEW_HPP
#define INFOSCREEN1VIEW_HPP

#include <gui_generated/infoscreen1_screen/InfoScreen1ViewBase.hpp>
#include <gui/infoscreen1_screen/InfoScreen1Presenter.hpp>

class InfoScreen1View : public InfoScreen1ViewBase
{
public:
    InfoScreen1View();
    virtual ~InfoScreen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // INFOSCREEN1VIEW_HPP
