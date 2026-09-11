#ifndef B_OPTIONVIEW_HPP
#define B_OPTIONVIEW_HPP

#include <gui_generated/b_option_screen/B_optionViewBase.hpp>
#include <gui/b_option_screen/B_optionPresenter.hpp>
#include <touchgfx/widgets/canvas/Circle.hpp>
#include <touchgfx/widgets/canvas/PainterRGB565.hpp>
#include <touchgfx/widgets/Image.hpp>

class B_optionView : public B_optionViewBase
{
public:
    B_optionView();
    virtual ~B_optionView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /* 매 프레임 호출 - 게이지 채움/비움 왕복 애니메이션 (데모) */
    virtual void handleTickEvent();

protected:
    /* 게이지 지오메트리 (b_gauge_track.png과 동일한 값)
     * TouchGFX Circle 각도 기준: 0도 = 12시 방향, 시계방향 증가.
     * 트랙은 -120도 ~ +120도 (아래쪽이 열린 240도 게이지) */
    static const int16_t ARC_START = -120;
    static const int16_t ARC_SPAN  = 240;
    static const uint16_t HALF_PERIOD_TICKS = 180;   /* 편도 180틱 ≈ 76Hz에서 2.4초 */

    /* 값 아크: 코드가 실시간으로 그리는 캔버스 위젯 (PNG 프레임 불필요) */
    touchgfx::Circle        arcSw;
    touchgfx::Circle        arcDet;
    touchgfx::PainterRGB565 painterSw;
    touchgfx::PainterRGB565 painterDet;

    /* 아크 끝점을 따라다니는 노브 */
    touchgfx::Image knobSw;
    touchgfx::Image knobDet;

    uint16_t animTick;

    void initGauge(touchgfx::Circle &arc, touchgfx::PainterRGB565 &painter,
                   const touchgfx::Image &track);
    void setGauge(touchgfx::Circle &arc, touchgfx::Image &knob,
                  const touchgfx::Image &track, int16_t sweepDeg);
};

#endif // B_OPTIONVIEW_HPP
