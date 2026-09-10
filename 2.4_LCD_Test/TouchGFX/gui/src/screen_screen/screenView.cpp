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
     * 실제 펌웨어에서는 이 줄을 지우고 CAN 이벤트가 setEmergency()를 부른다.
     * NOTE: 새 UI에는 아직 Emergency 위젯이 없어 잠시 꺼둠 - Designer에서
     * 위젯을 다시 배치한 뒤 아래 EMERGENCY-TODO 두 곳을 채우고 주석 해제. */
    /* setEmergency(true); */
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

    /* EMERGENCY-TODO(1/2): Designer에서 Emergency 위젯(테두리 Box들, 문구
     * TextArea 등)을 다시 배치하면 여기서 setVisible(active)로 일괄 on/off,
     * 꺼질 때는 각 위젯 invalidate()로 사라진 영역을 재그리기.
     * 이전 UI 예시:
     *   box1.setVisible(active); ... textArea9.setVisible(active);
     *   if (!active) { box1.invalidate(); ... }                       */
    if (active)
    {
        applyEmergencyAlpha((uint8_t)emergencyAlpha);   /* setAlpha + invalidate 포함 */
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
    /* EMERGENCY-TODO(2/2): Emergency 위젯을 다시 배치하면 각 위젯에
     *   위젯.setAlpha(alpha); 위젯.invalidate();
     * 를 나열. (이전 UI: box1~box4, boxWithBorder1, textArea9)              */
    (void)alpha;
}
