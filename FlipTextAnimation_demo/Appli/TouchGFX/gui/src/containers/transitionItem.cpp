#include <gui/containers/transitionItem.hpp>

transitionItem::transitionItem()
{

}

void transitionItem::initialize()
{
    transitionItemBase::initialize();
}

void transitionItem::setIcon(uint16_t iconId)
{
    transitionIcon.setBitmap(Bitmap(iconId));
}

void transitionItem::setName(uint16_t textId)
{
    transitionName.setTypedText(TypedText(textId));
}

void transitionItem::fade(uint8_t endAlpha, uint16_t duration, EasingEquation alphaProgressionEquation)
{
    transitionIcon.startFadeAnimation(endAlpha, duration, alphaProgressionEquation);
}
