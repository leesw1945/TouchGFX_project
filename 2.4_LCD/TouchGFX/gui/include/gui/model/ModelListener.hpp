#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <stdint.h>

/* Model이 CAN 수신 데이터의 변화를 Presenter에 알리는 인터페이스.
 * 기본 구현은 비어 있으므로 관심 있는 콜백만 재정의하면 된다.
 * 값이 바뀌었을 때만 호출되고(매 프레임 아님), 화면 진입 직후에는 현재 값 전부가 한 번 통지된다. */
class ModelListener
{
public:
    ModelListener() : model(0) {}

    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    /* 0x02 표시 데이터. valid=false면 통신 두절(1.5초 이상 미수신) → 화면은 대시로 */
    virtual void onDisplayData(bool valid, uint8_t unit, uint16_t sidMm, int16_t armDeg, int16_t detDeg)
    {
        (void)valid; (void)unit; (void)sidMm; (void)armDeg; (void)detDeg;
    }

    /* 0x03 ARM 높이. valid=false면 두절 또는 미수신(구버전 메인) → 대시 */
    virtual void onHeightData(bool valid, uint16_t heightMm)
    {
        (void)valid; (void)heightMm;
    }

    /* 0x04 상태. true = EMERGENCY (상태바 빨강) */
    virtual void onEmergency(bool emergency)
    {
        (void)emergency;
    }

    /* 동작 중인 카드 비트마스크: bit0 SID, bit1 ARM 높이, bit2 ARM 각도, bit3 Detector 각도 */
    virtual void onActiveCards(uint8_t mask)
    {
        (void)mask;
    }

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
