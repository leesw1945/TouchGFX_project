#include <gui/containers/menuIconCustomContainer.hpp>
#include <gui/common/MenuItems.hpp>

MenuIconCustomContainer::MenuIconCustomContainer()
{

}

void MenuIconCustomContainer::initialize()
{
    MenuIconCustomContainerBase::initialize();
}

void MenuIconCustomContainer::setIndex(int16_t index)
{
    // Set the icon bitmap according to the position in the list
    iconImage.setBitmap(menuItems[index].iconId);
    iconImage.invalidate();
}

void MenuIconCustomContainer::invalidateContent(void) const
{
    iconImage.invalidate();
}
