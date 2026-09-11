#ifndef A_OPTIONVIEW_HPP
#define A_OPTIONVIEW_HPP

#include <gui_generated/a_option_screen/A_optionViewBase.hpp>
#include <gui/a_option_screen/A_optionPresenter.hpp>

class A_optionView : public A_optionViewBase
{
public:
    A_optionView();
    virtual ~A_optionView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /* 매 프레임 자동 호출 - 컨테이너들의 애니메이션에 틱을 전달 */
    virtual void handleTickEvent();

    /* 나중에 Presenter(CAN 이벤트)가 호출할 진입점 */
    void showConnecting(bool on);
    void showEmergency(bool on);

protected:
};

#endif // A_OPTIONVIEW_HPP
