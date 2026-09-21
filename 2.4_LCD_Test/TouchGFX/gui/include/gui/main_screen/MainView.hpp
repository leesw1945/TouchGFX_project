#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <touchgfx/widgets/canvas/Circle.hpp>
#include <touchgfx/widgets/canvas/PainterRGB565.hpp>
#include <touchgfx/widgets/canvas/PainterRGB565LinearGradient.hpp>

/* 확정 UI(SU-4100 디자인) 메인 화면.
 *
 * Designer(2.4_LCD_Test.touchgfx)에는 정적 요소(배경, 상태바, 카드, 배지, 텍스트, 기호 이미지)만 있고,
 * 게이지 원호와 값 배치 규칙은 이 클래스가 담당한다.
 *
 * 외부(Presenter)에서 쓰는 진입점:
 *   setState()        READY(연두 #36D260) / EMERGENCY(빨강 #EB4B4B) 상태바
 *   setSidCm()        SID를 cm 정수로 표시 (인치 기호 없음, 단위 "cm")
 *   setSidInch()      SID를 inch 소수 1자리로 표시 (인치 기호 표시, 단위 "inch"). 인자는 1/10 inch 단위
 *   setArmUpDownCm()/setArmUpDownInch()   ARM UP/DOWN 카드, 위와 동일 규칙
 *   setArmDeg()/setDetectorDeg()          회전 각도: 숫자는 게이지 중앙에 항상 가운데 정렬, 원호가 값에 따라 채워짐
 *   setActive()       동작 중 카드: 테두리 파랑(card_act.png) + 값 파랑(#0077C0) + 게이지 파랑. 정지: 회색 계열
 */
class MainView : public MainViewBase
{
public:
    enum State { STATE_READY, STATE_EMERGENCY };
    enum Card  { CARD_SID, CARD_ARM_UD, CARD_ARM_DEG, CARD_DET_DEG };

    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /* [2.4_LCD_Test 전용] 조이스틱 CENTER(키 0)로 READY/EMERGENCY 토글.
     * 실제 프로젝트에서는 CAN으로 받은 Emergency 값으로 Presenter가 setState()를 호출한다. */
    virtual void handleKeyEvent(uint8_t key);

    /* [2.4_LCD_Test 전용] ARM 게이지 왕복 데모 애니메이션. 실제 프로젝트에서는 수신 값으로 setArmDeg()만 호출. */
    virtual void handleTickEvent();

    void setState(State s);
    void setSidCm(uint16_t cm);
    void setSidInch(uint16_t inch10);
    void setArmUpDownCm(uint16_t cm);
    void setArmUpDownInch(uint16_t inch10);
    void setArmDeg(int16_t deg);
    void setDetectorDeg(int16_t deg);
    void setActive(Card card, bool active);

protected:
    /* 디자인 원본(SU-4100_1/2.png) 계측값 */
    static const int16_t CARD_W        = 107;   /* 카드 실제 폭 (테두리 포함) */
    static const int16_t CARD_LEFT_X   = 8;
    static const int16_t CARD_RIGHT_X  = 124;
    static const int16_t GAUGE_ROW_Y   = 180;   /* 아래 카드 행의 카드 상단 y */
    /* 기호(인치 = Montserrat Bold 22의 U+2033, 도 = Bold 16의 U+00B0)는 이미지가 아닌 텍스트. 숫자 오른끝 기준 오프셋 */
    static const int16_t MARK_INCH_DX  = -1;     /* 숫자 오른끝 -> 인치 기호 TextArea X */
    static const int16_t MARK_INCH_DY  = -1;     /* 값 TextArea Y -> 인치 기호 TextArea Y */
    static const int16_t MARK_DEG_DX   = 0;     /* 숫자 오른끝 -> 도 기호 TextArea X */
    static const int16_t MARK_DEG_DY   = -2;    /* 값 TextArea Y -> 도 기호 TextArea Y */
    static const int16_t DOT_GAP       = 15;    /* 상태 텍스트 왼끝(advance 기준) -> 점 이미지 X. 시뮬레이터 계측으로 보정 */

    /* 게이지: 카드 좌표계 기준 중심 (53.5, 84), 선 중심 반지름 40, 선 굵기 8.
     * 각도는 TouchGFX 규약(0도 = 12시, 시계방향). 트랙은 시각적으로 +-128도인데
     * 라운드 캡(반지름 4 -> 약 5.7도)이 양끝에 붙으므로 실제 원호는 +-122도로 그린다. */
    static const int16_t ARC_START       = -122;
    static const int16_t ARC_END         = 122;
    static const int16_t GAUGE_VALUE_MAX = 200;   /* 이 값에서 원호가 가득 참 - 실제 범위 확정 시 변경 */

    touchgfx::Circle trackArm;
    touchgfx::Circle progArm;
    touchgfx::Circle trackDet;
    touchgfx::Circle progDet;
    touchgfx::PainterRGB565               trackPainter;   /* 빈 트랙 #E6E6E6 */
    touchgfx::PainterRGB565LinearGradient bluePainter;    /* 동작 중: 아래 #0077C0 -> 위 #62B8ED */
    touchgfx::PainterRGB565LinearGradient grayPainter;    /* 정지: 아래 #8C8C8C -> 위 #BDBDBD (트랙과 구분) */

    bool cardActive[4];
    State state;

    /* 데모 애니메이션 상태 (테스트 전용) */
    static const uint16_t DEMO_HALF_PERIOD_TICKS = 240;   /* 편도 240틱 (60Hz 시뮬 기준 4초) */
    uint16_t demoTick;
    int16_t  demoLastDeg;

    void initGauge(touchgfx::Circle& track, touchgfx::Circle& prog, int16_t cardX);
    void setLinearValue(touchgfx::TextAreaWithOneWildcard& val, touchgfx::Unicode::UnicodeChar* buf, uint16_t bufSize,
                        touchgfx::TextArea& mark, touchgfx::TextArea& unit, int16_t cardX, uint16_t value, bool inch);
    void setAngleValue(touchgfx::TextAreaWithOneWildcard& val, touchgfx::Unicode::UnicodeChar* buf, uint16_t bufSize,
                       touchgfx::TextArea& mark, touchgfx::Circle& prog, int16_t cardX, int16_t deg);
    static void buildGradient(uint32_t* tex, uint32_t fromRGB, uint32_t toRGB);
};

#endif // MAINVIEW_HPP
