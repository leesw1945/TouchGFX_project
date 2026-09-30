#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#ifndef SIMULATOR
extern "C" {
#include "main.h"       /* HAL_GetTick */
#include "can_app.h"
#include "key_scan.h"
}
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
      lastHeightValid(false), lastHeight(0),
      lastEmergency(false),
      lastActive(0)
{
    for (int i = 0; i < CARD_COUNT; i++)
    {
        changeTick[i] = 0u - 100000u;   /* 부팅 직후 "방금 바뀜"으로 오판하지 않게 먼 과거로 */
    }
#ifdef SIMULATOR
    simMs = 0;
    simNext = 0;
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

    const BuckHeightData* h = CAN_App_GetHeightData();
    s.heightValid = (h->valid != 0) && ((now - h->last_rx_tick) <= CAN_DISPLAY_TIMEOUT_MS);
    s.heightMm    = h->height_mm;

    s.emergency = (CAN_App_IsEmergency() != 0);
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
                               (s.heightValid && s.heightMm != lastHeight);
    if (refresh || heightChanged)
    {
        modelListener->onHeightData(s.heightValid, s.heightMm);
        lastHeightValid = s.heightValid;
        lastHeight = s.heightMm;
    }

    if (refresh || s.emergency != lastEmergency)
    {
        modelListener->onEmergency(s.emergency);
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
    static const int16_t vmin[4]  = {  500,  800, -30, -45 };
    static const int16_t vmax[4]  = { 1800, 1500, 120,  45 };
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
        simSnap.emergency = false;
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
    simSnap.emergency   = (simPhase == 3 && simPause > 0);
    simSnap.keys        = 0;
}
#endif
