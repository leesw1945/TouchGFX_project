#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>

class ModelListener;

/* CAN 수신 데이터(can_app.c)를 매 프레임 읽어, 변화가 있을 때만 Presenter에 알린다.
 *
 * - 통신 두절: 0x02/0x03이 CAN_DISPLAY_TIMEOUT_MS(1.5초) 동안 없으면 valid=false로 통지
 * - 동작 중 판단(카드 파란 강조): 값이 바뀐 뒤 ACTIVE_HOLD_MS 동안, 또는 그 동작에
 *   해당하는 키를 누르고 있는 동안 (EMERGENCY 중에는 키 강조 없음)
 * - 시뮬레이터(PC) 빌드에서는 CAN 대신 내장 시나리오로 값을 만들어 화면을 검증한다 */
class Model
{
public:
    enum Card
    {
        CARD_SID = 0,
        CARD_ARM_UD,
        CARD_ARM_DEG,
        CARD_DET_DEG,
        CARD_COUNT
    };

    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    /* 화면 진입 시 Presenter가 호출: 다음 tick에서 현재 값 전부를 다시 통지한다 */
    void requestRefresh()
    {
        refresh = true;
    }

protected:
    static const uint32_t ACTIVE_HOLD_MS = 600;   /* 값이 멈춘 뒤 강조를 유지하는 시간 */

    /* 한 프레임에 읽은 소스 값 묶음 */
    struct Snapshot
    {
        bool     dispValid;
        uint8_t  unit;
        uint16_t sidMm;
        int16_t  armDeg;
        int16_t  detDeg;
        bool     heightValid;
        uint16_t heightMm;
        bool     emergency;
        uint16_t keys;        /* 물리 키 비트마스크 (1 = 눌림) */
    };

    ModelListener* modelListener;
    bool           refresh;

    /* 마지막으로 통지한 값 (변화 감지용) */
    bool     lastDispValid;
    uint8_t  lastUnit;
    uint16_t lastSid;
    int16_t  lastArm;
    int16_t  lastDet;
    bool     lastHeightValid;
    uint16_t lastHeight;
    bool     lastEmergency;
    uint8_t  lastActive;
    uint32_t changeTick[CARD_COUNT];   /* 카드별 값이 마지막으로 바뀐 시각(ms) */

    uint32_t nowMs();
    void     readSource(uint32_t now, Snapshot& s);

#ifdef SIMULATOR
    /* PC 시뮬레이터용 내장 시나리오 (dbg_cmd.c의 sim과 같은 순서) */
    uint32_t simMs;
    uint32_t simNext;
    uint8_t  simPhase;
    uint8_t  simPause;
    uint8_t  simUnit;
    bool     simTowardMax[4];
    int16_t  simVal[4];
    bool     simLinkLost;
    uint32_t simLinkLostAt;
    Snapshot simSnap;
    void     simulate(uint32_t now);
#endif
};

#endif // MODEL_HPP
