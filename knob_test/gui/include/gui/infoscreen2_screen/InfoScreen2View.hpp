#ifndef INFOSCREEN2VIEW_HPP
#define INFOSCREEN2VIEW_HPP

#include <gui_generated/infoscreen2_screen/InfoScreen2ViewBase.hpp>
#include <gui/infoscreen2_screen/InfoScreen2Presenter.hpp>

class InfoScreen2View : public InfoScreen2ViewBase
{
public:
    InfoScreen2View();
    virtual ~InfoScreen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // INFOSCREEN2VIEW_HPP
