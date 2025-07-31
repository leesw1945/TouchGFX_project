#ifndef AUTOPOSTIONSETVIEW_HPP
#define AUTOPOSTIONSETVIEW_HPP

#include <gui_generated/autopostionset_screen/AutoPostionSetViewBase.hpp>
#include <gui/autopostionset_screen/AutoPostionSetPresenter.hpp>

class AutoPostionSetView : public AutoPostionSetViewBase
{
public:
    AutoPostionSetView();
    virtual ~AutoPostionSetView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // AUTOPOSTIONSETVIEW_HPP
