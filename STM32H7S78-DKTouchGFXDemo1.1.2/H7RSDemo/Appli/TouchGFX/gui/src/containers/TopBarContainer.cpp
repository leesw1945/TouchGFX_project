#include <gui/containers/TopBarContainer.hpp>
#include <gui/common/Utils.hpp>

TopBarContainer::TopBarContainer():
    neoChromActive(true)
{

}

void TopBarContainer::initialize()
{
    TopBarContainerBase::initialize();
}

void TopBarContainer::updateFPS(int16_t fps)
{
    Unicode::snprintf(fpsValueBuffer, FPSVALUE_SIZE, "%d", fps);
    fpsValue.invalidate();
}

void TopBarContainer::updateMCU(uint16_t mcuLoad)
{
    Unicode::snprintf(mcuValueBuffer, MCUVALUE_SIZE, "%d", mcuLoad);
    mcuValue.invalidate();
}

void TopBarContainer::toggleNeoChrom()
{
    neoChromActive ^= true;

    Utils::activateNeoChrom(neoChromActive);

    if (neoChromActive)
    {
        neoChromSelectImage.startMoveAnimation(684, 16, 5);
        neoChromOnText.setColor(colortype(0x050A16));
        neoChromOffText.setColor(colortype(0x848A9C));
    }
    else
    {
        neoChromSelectImage.startMoveAnimation(736, 16, 5);
        neoChromOnText.setColor(colortype(0x848A9C));
        neoChromOffText.setColor(colortype(0x050A16));
    }
}
