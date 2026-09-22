#ifndef MAINPRESENTER_HPP
#define MAINPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MainView;

/* 확정 UI(SU-4100) 메인 화면 Presenter.
 * 실제 프로젝트(2.4_LCD)에서는 Model이 CAN 값을 받아 여기로 알리고,
 * Presenter가 view.setSidCm()/setArmDeg()/setActive()/setState()를 호출하게 된다. */
class MainPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MainPresenter(MainView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~MainPresenter() {}

private:
    MainPresenter();

    MainView& view;
};

#endif // MAINPRESENTER_HPP
