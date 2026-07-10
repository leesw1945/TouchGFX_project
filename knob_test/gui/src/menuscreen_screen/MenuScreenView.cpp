#include <gui/MenuScreen_screen/MenuScreenView.hpp>
#include <gui/common/MenuItems.hpp>

MenuScreenView::MenuScreenView()
{

}

void MenuScreenView::setupScreen()
{
    MenuScreenViewBase::setupScreen();

    menuScrollWheel.animateToItem(presenter->getSelectedDemoNumber(), 0);

    // Hide the widgets that will animate in when the wheel stops
    titleTextArea.setVisible(false);
    gradientImage.setVisible(false);

    presenter->setAmbientLightRGB(255, 128, 128);
}

void MenuScreenView::tearDownScreen()
{
    MenuScreenViewBase::tearDownScreen();
}

void MenuScreenView::menuScrollWheelUpdateItem(MenuIconCustomContainer& item, int16_t itemIndex)
{
    // Update the menu elements based on their indicies
    item.setIndex(itemIndex);
}

void MenuScreenView::handleTickEvent()
{
    MenuScreenViewBase::handleTickEvent();

    if (menuScrollWheel.isAnimating() && titleTextArea.isVisible())
    {
        titleTextArea.setVisible(false);
        titleTextArea.invalidate();
        gradientImage.setVisible(false);
        gradientImage.invalidate();
    }

    // Detect when menu wheel stops animating to an icon
    if (!menuScrollWheel.isAnimating() && !titleTextArea.isVisible())
    {
        MenuItem activeItem = menuItems[menuScrollWheel.getSelectedItem()];

        // Start title text animation
        titleTextArea.setTypedText(touchgfx::TypedText(activeItem.titleId));
        titleTextArea.setAlpha(0);
        titleTextArea.startFadeAnimation(255, 10, EasingEquations::quadEaseIn);
        titleTextArea.setVisible(true);

        // Start color gradient animation
        gradientImage.setBitmap(activeItem.gradientId);
        gradientImage.setX(240);
        gradientImage.startMoveAnimation(240 - gradientImage.getWidth(), 0, 10, EasingEquations::quadEaseIn);
        gradientImage.setVisible(true);
    }
}

void MenuScreenView::handleClickEvent(const ClickEvent& event)
{
    if (event.getType() == touchgfx::ClickEvent::ClickEventType::RELEASED &&
            !menuScrollWheel.isAnimating() &&
            event.getY() >= 80 &&
            event.getY() < 160)
    {
        // Enter sub demo, when the menu is standing still and the selected icon is clicked
        enter();
    }
    else
    {
        // Otherwise, propagate the click event to the default handling
        MenuScreenViewBase::handleClickEvent(event);
    }
}

void MenuScreenView::rollMenu(int16_t change)
{
    // Animate the menu wheel to a new position, if it is valid
    int newIndex = menuScrollWheel.getSelectedItem() + change;
    if (newIndex >= 0 && newIndex < menuScrollWheel.getNumberOfItems())
    {
        menuScrollWheel.animateToItem(newIndex, 20);

        // Hide the title and color gradient during the menu animation
        titleTextArea.setVisible(false);
        titleTextArea.invalidate();
        gradientImage.setVisible(false);
        gradientImage.invalidate();
    }
}

void MenuScreenView::scrollDown()
{
    rollMenu(1);
}

void MenuScreenView::scrollUp()
{
    rollMenu(-1);
}

void MenuScreenView::enter()
{
    // Update the parameter in the model, to be able to start at the same position when returning to the menu
    presenter->setSelectedDemoNumber(menuScrollWheel.getSelectedItem());

    // Use the text for the selected icon to determine which screen to switch to
    switch (menuItems[menuScrollWheel.getSelectedItem()].titleId)
    {
    case T_MENU_TEMPERATURE:
        goToTemperature();
        break;
    case T_MENU_BATTERY:
        goToBattery();
        break;
    case T_MENU_VOLUME:
        goToVolume();
        break;
    case T_MENU_GUAGE:
        goToGauge();
        break;
    case T_MENU_LIGHT:
        goToLight();
        break;
    case T_MENU_INFO:
        goToInfo();
        break;
    default:
        break;
    }
}
