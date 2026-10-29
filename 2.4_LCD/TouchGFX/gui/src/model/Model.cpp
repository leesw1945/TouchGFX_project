#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#ifndef SIMULATOR
extern "C" {
#include "main.h"       /* HAL_GetTick */
#include "can_app.h"
#include "key_scan.h"
}
/* OP 보드 0x0A 페이로드가 C++ 쪽에서도 4바이트로 잡히는지 확인 (can_app.c의 C 쪽 검사와 쌍) */
static_assert(sizeof(OP_To_Bucky_Key_Payload_t) == 4, "OP_To_Bucky_Key_Payload_t must be 4 bytes");
#endif

/* 동작 키 → 카드 대응 (물리 cKEY 번호. LCD를 마주본 기준 왼쪽 열 = 0~5, 오른쪽 열 = 6~11)
 *   ARM 높이 : KEY0 ARM_UP,           KEY6 ARM_DOWN
 *   ARM 각도 : KEY1 ARM_LEFT_ROT,     KEY7 ARM_RIGHT_ROT
 *   SID      : KEY2 TUBE_RIGHT_SLIDE, KEY8 TUBE_LEFT_SLIDE, KEY3 DET_RIGHT_SLIDE, KEY9 DET_LEFT_SLIDE
 *   DET 각도 : KEY4 DET_LEFT_ROT,     KEY10 DET_RIGHT_ROT
 * 키를 누르는 동안 해당 카드를 즉시 "동작 중"으로 표시한다 (값이 바뀔 때까지의 지연 제거).
 * 키 기반 강조를 끄려면 마스크를 0으로 두면 된다. */
static const uint16_t KEYS_SID     = (1u << 2) | (1u << 8) | (1u << 3) | (1u << 9);
static const uint16_t KEYS_ARM_UD  = (1u << 0) | (1u << 6);
static const uint16_t KEYS_ARM_DEG = (1u << 1) | (1u << 7);
static const uint16_t KEYS_DET_DEG = (1u << 4) | (1u << 10);

Model::Model()
    : modelListener(0),
      refresh(true),
      lastDispValid(false), lastUnit(0), lastSid(0), lastArm(0), lastDet(0),
      lastHeightValid(false), lastHeightUnit(0), lastHeight(0),
      lastConnected(false), lastEmergency(false),
      lastActive(0)
{
    for (int i = 0; i < CARD_COUNT; i++)
    {
        changeTick[i] = 0u - 100000u;   /* 부팅 직후 "방금 바뀜"으로 오판하지 않게 먼 과거로 */
    }
#ifdef SIMULATOR
    simMs = 0;
    simNext = 4000;   /* 처음 4초는 미수신(DISCONNECTED) — 부팅 직후 OP가 아직 안 보내는 상황 */
    simPhase = 0;
    simPause = 0;
    simUnit = 0;
    simLinkLost = false;
    simLinkLostAt = 0;
    static const int16_t init[4] = { 1000, 800, -30, -45 };
    for (int i = 0; i < 4; i++)
    {
        simTowardMax[i] = true;
        simVal[i] = init[i];
    }
    simSnap.dispValid = false;
    simSnap.heightValid = false;
    simSnap.connected = false;
    simSnap.emergency = false;
    simSnap.keys = 0;
    simSnap.unit = 0;
    simSnap.sidMm = 0;
    simSnap.armDeg = 0;
    simSnap.detDeg = 0;
    simSnap.heightMm = 0;
#endif
}

uint32_t Model::nowMs()
{
#ifndef SIMULATOR
    return HAL_GetTick();
#else
    return simMs;
#endif
}

void Model::readSource(uint32_t now, Snapshot& s)
{
#ifndef SIMULATOR
    const BuckDisplayData* d = CAN_App_GetDisplayData();
    s.dispValid = (d->valid != 0) && ((now - d->last_rx_tick) <= CAN_DISPLAY_TIMEOUT_MS);
    s.unit      = d->unit;
    s.sidMm     = d->sid_mm;
    s.armDeg    = d->arm_angle_deg;
    s.detDeg    = d->det_angle_deg;

    /* 0x0A (OP 보드 정의 페이로드): 단위 + Emergency + ARM 높이 */
    const BuckPayloadData* p = CAN_App_GetPayloadData();
    const bool payloadValid = (p->valid != 0) && ((now - p->last_rx_tick) <= CAN_DISPLAY_TIMEOUT_MS);
    s.heightValid = payloadValid;
    s.heightMm    = p->payload.Swivel_Height;
    if (payloadValid)
    {
        s.unit = (uint8_t)p->payload.Unit;   /* 0x0A가 살아 있으면 그 단위 우선, 아니면 0x02의 단위 */
    }

    /* 상태 표시: 마지막으로 받은 Emergency 값(CAN_App_IsEmergency). 0x0A가 끊겨도 바뀌지 않는다
     * (EMERGENCY 중 두절 → EMERGENCY 유지, READY 중 두절 → READY 유지. 2026-10-29 결정).
     * 키 차단(app_main)과 절전 금지(sleep_app)도 같은 함수를 쓰므로 화면과 항상 일치한다 */
    s.emergency = (CAN_App_IsEmergency() != 0);
    /* 연결 여부: 0x02 또는 0x0A 수신 중. 아니면 상단 바 DISCONNECTED (EMERGENCY보다 우선 표시) */
    s.connected = (CAN_App_IsConnected() != 0);
    s.keys      = KEY_GetStableMask();
#else
    simulate(now);
    s = simSnap;
#endif
}

void Model::tick()
{
#ifdef SIMULATOR
    simMs += 16;   /* 시뮬레이터는 약 60fps 가정 */
#endif
    if (!modelListener)
    {
        return;
    }

    const uint32_t now = nowMs();
    Snapshot s;
    readSource(now, s);

    /* 값이 바뀐 시각 기록 (동작 중 판단용). 유효한 값끼리 비교했을 때만 */
    if (s.dispValid && lastDispValid)
    {
        if (s.sidMm != lastSid)   changeTick[CARD_SID]     = now;
        if (s.armDeg != lastArm)  changeTick[CARD_ARM_DEG] = now;
        if (s.detDeg != lastDet)  changeTick[CARD_DET_DEG] = now;
    }
    if (s.heightValid && lastHeightValid && s.heightMm != lastHeight)
    {
        changeTick[CARD_ARM_UD] = now;
    }

    /* 표시 데이터 통지 (바뀐 경우만) */
    const bool dispChanged = (s.dispValid != lastDispValid) ||
                             (s.dispValid && (s.unit != lastUnit || s.sidMm != lastSid ||
                                              s.armDeg != lastArm || s.detDeg != lastDet));
    if (refresh || dispChanged)
    {
        modelListener->onDisplayData(s.dispValid, s.unit, s.sidMm, s.armDeg, s.detDeg);
        lastDispValid = s.dispValid;
        lastUnit = s.unit;
        lastSid = s.sidMm;
        lastArm = s.armDeg;
        lastDet = s.detDeg;
    }

    const bool heightChanged = (s.heightValid != lastHeightValid) ||
                               (s.heightValid && (s.heightMm != lastHeight || s.unit != lastHeightUnit));
    if (refresh || heightChanged)
    {
        modelListener->onHeightData(s.heightValid, s.unit, s.heightMm);
        lastHeightValid = s.heightValid;
        lastHeightUnit  = s.unit;
        lastHeight      = s.heightMm;
    }

    if (refresh || s.connected != lastConnected || s.emergency != lastEmergency)
    {
        modelListener->onStatus(s.connected, s.emergency);
        lastConnected = s.connected;
        lastEmergency = s.emergency;
    }

    /* 동작 중 카드: 값이 최근 ACTIVE_HOLD_MS 안에 바뀌었거나, 해당 키를 누르고 있는 동안.
     * 값이 유효할 때만(두절 중 대시 위에 강조가 뜨지 않게), 키 강조는 EMERGENCY가 아닐 때만 */
    uint8_t active = 0;
    if (s.dispValid)
    {
        if ((now - changeTick[CARD_SID])     < ACTIVE_HOLD_MS) active |= (1u << CARD_SID);
        if ((now - changeTick[CARD_ARM_DEG]) < ACTIVE_HOLD_MS) active |= (1u << CARD_ARM_DEG);
        if ((now - changeTick[CARD_DET_DEG]) < ACTIVE_HOLD_MS) active |= (1u << CARD_DET_DEG);
        if (!s.emergency)
        {
            if (s.keys & KEYS_SID)     active |= (1u << CARD_SID);
            if (s.keys & KEYS_ARM_DEG) active |= (1u << CARD_ARM_DEG);
            if (s.keys & KEYS_DET_DEG) active |= (1u << CARD_DET_DEG);
        }
    }
    if (s.heightValid)
    {
        if ((now - changeTick[CARD_ARM_UD]) < ACTIVE_HOLD_MS) active |= (1u << CARD_ARM_UD);
        if (!s.emergency && (s.keys & KEYS_ARM_UD)) active |= (1u << CARD_ARM_UD);
    }
    if (refresh || active != lastActive)
    {
        modelListener->onActiveCards(active);
        lastActive = active;
    }

    refresh = false;
}

#ifdef SIMULATOR
/* PC 시뮬레이터 시나리오: SID → 높이 → ARM 각도 → DET 각도 순서로 하나씩 움직이고
 * 단계 사이 1.5초 정지, 한 바퀴 돌면 3초 통신 두절(대시) 후 단위를 바꿔 반복.
 * DET 단계의 정지 구간에서는 EMERGENCY 표시를 확인할 수 있게 상태를 켠다. */
void Model::simulate(uint32_t now)
{
    static const int16_t vmin[4]  = {  500,  400, -30, -45 };
    static const int16_t vmax[4]  = { 1800, 1700, 120,  45 };
    static const int16_t vstep[4] = {   50,   50,  10,   6 };

    if (now < simNext)
    {
        return;
    }
    simNext = now + 300;

    if (simLinkLost)
    {
        simSnap.dispValid = false;
        simSnap.heightValid = false;
        simSnap.connected = false;   /* emergency는 보드처럼 마지막 값 유지 (화면은 DISCONNECTED가 우선) */
        if ((now - simLinkLostAt) >= 3000)
        {
            simLinkLost = false;
            simUnit ^= 1;
        }
        return;
    }

    if (simPause)
    {
        if (--simPause == 0)
        {
            simPhase = (simPhase + 1) & 3;
            if (simPhase == 0)
            {
                simLinkLost = true;
                simLinkLostAt = now;
            }
        }
    }
    else
    {
        const int16_t target = simTowardMax[simPhase] ? vmax[simPhase] : vmin[simPhase];
        int16_t v = simVal[simPhase];
        if (v < target)
        {
            v = (int16_t)(v + vstep[simPhase]);
            if (v > target) v = target;
        }
        else if (v > target)
        {
            v = (int16_t)(v - vstep[simPhase]);
            if (v < target) v = target;
        }
        simVal[simPhase] = v;
        if (v == target)
        {
            simTowardMax[simPhase] = !simTowardMax[simPhase];
            simPause = 5;
        }
    }

    simSnap.dispValid   = true;
    simSnap.unit        = simUnit;
    simSnap.sidMm       = (uint16_t)simVal[0];
    simSnap.armDeg      = simVal[2];
    simSnap.detDeg      = simVal[3];
    simSnap.heightValid = true;
    simSnap.heightMm    = (uint16_t)simVal[1];
    simSnap.connected   = true;
    simSnap.emergency   = (simPhase == 3 && simPause > 0);
    simSnap.keys        = 0;
}
#endif
