#ifndef MENUBTN_HPP
#define MENUBTN_HPP

#include <gui_generated/containers/MenuBtnBase.hpp>

class MenuBtn : public MenuBtnBase
{
public:
    MenuBtn();
    virtual ~MenuBtn() {}

    virtual void initialize();
protected:
};

#endif // MENUBTN_HPP
