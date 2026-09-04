#include <gui/screen_screen/screenView.hpp>

screenView::screenView()
    : emergencyActive(false),
      emergencyAlpha(EMERGENCY_ALPHA_MAX),
      emergencyAlphaDir(-EMERGENCY_ALPHA_STEP)
{
}

void screenView::setupScreen()
{
    screenViewBase::setupScreen();

    /* 데모: 부팅 즉시 Emergency 깜빡임 시작.
     * 실제 펌웨어에서는 이 줄을 지우고 CAN 이벤트가 setEmergency()를 부른다. */
    setEmergency(true);
}

void screenView::tearDownScreen()
{
    screenViewBase::tearDownScreen();
}

void screenView::setEmergency(bool active)
{
    emergencyActive   = active;
    emergencyAlpha    = EMERGENCY_ALPHA_MAX;
    emergencyAlphaDir = -EMERGENCY_ALPHA_STEP;

    /* Emergency 그룹 위젯 6개를 한꺼번에 켜고/끄기 */
    box1.setVisible(active);
    box2.setVisible(active);
    box3.setVisible(active);
    box4.setVisible(active);
    boxWithBorder1.setVisible(active);
    textArea9.setVisible(active);

    if (active)
    {
        applyEmergencyAlpha((uint8_t)emergencyAlpha);   /* setAlpha + invalidate 포함 */
    }
    else
    {
        /* 꺼질 때는 위젯이 사라진 영역을 다시 그려야 하므로 invalidate만 */
        box1.invalidate();
        box2.invalidate();
        box3.invalidate();
        box4.invalidate();
        boxWithBorder1.invalidate();
        textArea9.invalidate();
    }
}

void screenView::handleTickEvent()
{
    if (!emergencyActive)
    {
        return;
    }

    /* 삼각파 호흡: MAX에서 어두워지다 MIN을 치면 방향을 뒤집는다 */
    emergencyAlpha += emergencyAlphaDir;
    if (emergencyAlpha <= EMERGENCY_ALPHA_MIN)
    {
        emergencyAlpha    = EMERGENCY_ALPHA_MIN;
        emergencyAlphaDir = EMERGENCY_ALPHA_STEP;
    }
    else if (emergencyAlpha >= EMERGENCY_ALPHA_MAX)
    {
        emergencyAlpha    = EMERGENCY_ALPHA_MAX;
        emergencyAlphaDir = -EMERGENCY_ALPHA_STEP;
    }

    applyEmergencyAlpha((uint8_t)emergencyAlpha);
}

void screenView::applyEmergencyAlpha(uint8_t alpha)
{
    /* setAlpha는 값만 바꾼다 - invalidate로 "이 영역 다시 그려" 표시까지 해야
     * 파셜 프레임버퍼가 해당 사각형들을 LCD로 재전송한다 */
    box1.setAlpha(alpha);
    box1.invalidate();
    box2.setAlpha(alpha);
    box2.invalidate();
    box3.setAlpha(alpha);
    box3.invalidate();
    box4.setAlpha(alpha);
    box4.invalidate();
    boxWithBorder1.setAlpha(alpha);
    boxWithBorder1.invalidate();
    textArea9.setAlpha(alpha);
    textArea9.invalidate();
}
