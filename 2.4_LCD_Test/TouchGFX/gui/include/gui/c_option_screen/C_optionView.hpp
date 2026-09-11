#ifndef C_OPTIONVIEW_HPP
#define C_OPTIONVIEW_HPP

#include <gui_generated/c_option_screen/C_optionViewBase.hpp>
#include <gui/c_option_screen/C_optionPresenter.hpp>

class C_optionView : public C_optionViewBase
{
public:
    C_optionView();
    virtual ~C_optionView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // C_OPTIONVIEW_HPP
