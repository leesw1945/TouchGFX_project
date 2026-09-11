#include <gui/a_option_screen/A_optionView.hpp>

A_optionView::A_optionView()
{
}

void A_optionView::setupScreen()
{
    A_optionViewBase::setupScreen();

    /* 컨테이너 표시 여부는 Designer의 Visible 체크(생성 코드)를 그대로 따른다.
     * 스피너/호흡 애니메이션은 각자 보이는 동안에만 동작하므로 여기서 할 일 없음.
     * 실제 펌웨어에서는 CAN 이벤트 → Presenter가 showConnecting()/showEmergency() 호출 */
}

void A_optionView::tearDownScreen()
{
    A_optionViewBase::tearDownScreen();
}

void A_optionView::handleTickEvent()
{
    /* 각 컨테이너가 내부에서 isVisible()을 확인하므로 항상 전달해도 안전 */
    Connecting_cnt.tick();
    Emergency_cnt.tick();
}

void A_optionView::showConnecting(bool on)
{
    Connecting_cnt.setVisible(on);
    Connecting_cnt.invalidate();
}

void A_optionView::showEmergency(bool on)
{
    Emergency_cnt.setActive(on);
}
