#ifndef MAINPRESENTER_HPP
#define MAINPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MainView;

/* 확정 UI(SU-4100) 메인 화면 Presenter.
 * Model이 CAN 값의 변화를 알리면 단위를 변환해 View의 setter를 호출한다.
 *   - 거리(SID, ARM 높이)는 mm로 오며 unit(0 cm / 1 inch)에 따라 cm 정수 또는 1/10 inch로 변환
 *   - 통신 두절(valid=false)이면 대시 표시
 *   - EMERGENCY면 상태바를 빨강으로
 *   - 동작 중 카드는 Model이 판단한 비트마스크대로 강조 */
class MainPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MainPresenter(MainView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~MainPresenter() {}

    virtual void onDisplayData(bool valid, uint8_t unit, uint16_t sidMm, int16_t armDeg, int16_t detDeg);
    virtual void onHeightData(bool valid, uint16_t heightMm);
    virtual void onEmergency(bool emergency);
    virtual void onActiveCards(uint8_t mask);

private:
    MainPresenter();

    MainView& view;
    uint8_t   unit;          /* 마지막 수신 단위 — 높이 표시에도 같은 단위를 쓴다 */
    bool      heightValid;
    uint16_t  heightMm;

    void showHeight();
    static uint16_t mmToCm(uint16_t mm);       /* 반올림 cm */
    static uint16_t mmToInch10(uint16_t mm);   /* 반올림 1/10 inch */
};

#endif // MAINPRESENTER_HPP
