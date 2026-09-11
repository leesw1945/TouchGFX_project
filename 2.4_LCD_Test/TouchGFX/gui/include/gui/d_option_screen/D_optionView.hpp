#ifndef D_OPTIONVIEW_HPP
#define D_OPTIONVIEW_HPP

#include <gui_generated/d_option_screen/D_optionViewBase.hpp>
#include <gui/d_option_screen/D_optionPresenter.hpp>
#include <touchgfx/widgets/canvas/Circle.hpp>
#include <touchgfx/widgets/canvas/PainterRGB565.hpp>
#include <touchgfx/widgets/Image.hpp>

class D_optionView : public D_optionViewBase
{
public:
    D_optionView();
    virtual ~D_optionView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /* 매 프레임 호출 - 게이지 채움/비움 왕복 애니메이션 (데모) */
    virtual void handleTickEvent();

protected:
    /* 게이지 각도 기준: 0도 = 12시, 시계방향. 트랙 = -120도 ~ +120도 */
    static const int16_t ARC_START = -120;
    static const int16_t ARC_SPAN  = 240;
    static const uint16_t HALF_PERIOD_TICKS = 180;   /* 편도 180틱 ≈ 76Hz에서 2.4초 */

    touchgfx::Circle        arcL;
    touchgfx::Circle        arcR;
    touchgfx::PainterRGB565 painterL;
    touchgfx::PainterRGB565 painterR;
    touchgfx::Image         knobL;
    touchgfx::Image         knobR;

    uint16_t animTick;

    void initGauge(touchgfx::Circle &arc, touchgfx::PainterRGB565 &painter,
                   touchgfx::Image &knob, const touchgfx::Image &track,
                   uint16_t knobBitmapId);
    void setGauge(touchgfx::Circle &arc, touchgfx::Image &knob,
                  const touchgfx::Image &track, int16_t sweepDeg);
};

#endif // D_OPTIONVIEW_HPP
