#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>

MainPresenter::MainPresenter(MainView& v)
    : view(v), unit(0), heightValid(false), heightMm(0)
{
}

void MainPresenter::activate()
{
    /* 화면이 만들어진 직후 Model이 가진 현재 값을 전부 받아 초기 화면을 채운다 */
    if (model)
    {
        model->requestRefresh();
    }
}

void MainPresenter::deactivate()
{
}

uint16_t MainPresenter::mmToCm(uint16_t mm)
{
    return (uint16_t)((mm + 5u) / 10u);
}

uint16_t MainPresenter::mmToInch10(uint16_t mm)
{
    /* 1 inch = 25.4 mm → 1/10 inch = 2.54 mm. 정수 연산: mm * 100 / 254, 반올림 */
    return (uint16_t)(((uint32_t)mm * 100u + 127u) / 254u);
}

void MainPresenter::onDisplayData(bool valid, uint8_t u, uint16_t sidMm, int16_t armDeg, int16_t detDeg)
{
    if (!valid)
    {
        view.setSidDash();
        view.setArmDegDash();
        view.setDetectorDegDash();
        return;
    }

    const bool unitChanged = (u != unit);
    unit = u;

    if (unit)
    {
        view.setSidInch(mmToInch10(sidMm));
    }
    else
    {
        view.setSidCm(mmToCm(sidMm));
    }
    view.setArmDeg(armDeg);
    view.setDetectorDeg(detDeg);

    if (unitChanged)
    {
        showHeight();   /* 높이 카드도 새 단위로 다시 표시 */
    }
}

void MainPresenter::onHeightData(bool valid, uint16_t mm)
{
    heightValid = valid;
    heightMm    = mm;
    showHeight();
}

void MainPresenter::showHeight()
{
    if (!heightValid)
    {
        view.setArmUpDownDash();
    }
    else if (unit)
    {
        view.setArmUpDownInch(mmToInch10(heightMm));
    }
    else
    {
        view.setArmUpDownCm(mmToCm(heightMm));
    }
}

void MainPresenter::onEmergency(bool emergency)
{
    view.setState(emergency ? MainView::STATE_EMERGENCY : MainView::STATE_READY);
}

void MainPresenter::onActiveCards(uint8_t mask)
{
    view.setActive(MainView::CARD_SID,     (mask & 0x01u) != 0);
    view.setActive(MainView::CARD_ARM_UD,  (mask & 0x02u) != 0);
    view.setActive(MainView::CARD_ARM_DEG, (mask & 0x04u) != 0);
    view.setActive(MainView::CARD_DET_DEG, (mask & 0x08u) != 0);
}
