#include <gui/containers/Emergency.hpp>

Emergency::Emergency()
    : alpha(ALPHA_MAX),
      alphaDir(-ALPHA_STEP)
{
}

void Emergency::initialize()
{
    EmergencyBase::initialize();
}

void Emergency::setActive(bool active)
{
    alpha    = ALPHA_MAX;
    alphaDir = -ALPHA_STEP;

    setVisible(active);
    if (active)
    {
        applyAlpha((uint8_t)alpha);
    }
    invalidate();   /* 켤 때는 새로 그리고, 끌 때는 사라진 영역을 지움 */
}

void Emergency::tick()
{
    if (!isVisible())
    {
        return;
    }

    alpha += alphaDir;
    if (alpha <= ALPHA_MIN)
    {
        alpha    = ALPHA_MIN;
        alphaDir = ALPHA_STEP;
    }
    else if (alpha >= ALPHA_MAX)
    {
        alpha    = ALPHA_MAX;
        alphaDir = -ALPHA_STEP;
    }

    applyAlpha((uint8_t)alpha);
}

void Emergency::applyAlpha(uint8_t a)
{
    /* 컨테이너 자체는 알파가 없으므로 자식 위젯들에 일괄 적용.
     * setAlpha 후 invalidate로 해당 영역 재전송을 요청해야 화면에 반영된다 */
    boxWithBorder1.setAlpha(a);
    boxWithBorder1.invalidate();
    image1.setAlpha(a);
    image1.invalidate();
    textArea1.setAlpha(a);
    textArea1.invalidate();
}
