#ifndef MENUICONCUSTOMCONTAINER_HPP
#define MENUICONCUSTOMCONTAINER_HPP

#include <gui_generated/containers/MenuIconCustomContainerBase.hpp>

class MenuIconCustomContainer : public MenuIconCustomContainerBase
{
public:
    MenuIconCustomContainer();
    virtual ~MenuIconCustomContainer() {}
    virtual void initialize();

    void setIndex(int16_t index);
    void invalidateContent(void) const;

protected:
};

#endif // MENUICONCUSTOMCONTAINER_HPP
