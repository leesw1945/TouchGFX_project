/**
  ******************************************************************************
  * @file    sleep_app.c
  * @brief   절전 구현 — 활동 감지와 백라이트 소등/점등
  *
  *          비교 규칙: 직전 값과 지금 값이 모두 유효(수신 중, 1.5초 타임아웃 안)할 때만 비교한다.
  *          그래서 통신 두절(대시) 자체나 두절 뒤 같은 값으로 돌아오는 것은 활동이 아니다.
  *          단위(Cm/Inch) 변경도 활동으로 치지 않는다 (요구 목록에 없음).
  *
  *          콘솔 sim 명령으로 넣은 값도 can_app의 수신 버퍼를 거치므로 똑같이 활동으로 잡힌다
  *          (OP 없이 책상에서 깨어나는지 시험할 때 쓸 수 있다).
  ******************************************************************************
  */
#include "sleep_app.h"
#include "main.h"
#include "key_scan.h"
#include "can_app.h"
#include "display_driver.h"
#include <stdio.h>

static uint32_t last_activity_tick;   /* 마지막 활동 시각 (HAL_GetTick) */
static uint8_t  sleeping;             /* 1 = 절전 중 */
static uint8_t  panel_want_on;        /* 패널 출력 목표 (1 = Display ON) */
static uint8_t  panel_is_on;          /* 실제로 명령이 나간 상태. 목표와 다르면 매 폴 재시도 (DMA 중이면 미뤄진다) */

/* 직전 폴에서 본 값 — 변화 감지용 */
static uint16_t key_last;             /* 키 마스크 (1 = 눌림) */
static uint8_t  disp_last_valid;      /* 0x02가 유효했는가 */
static uint16_t sid_last;
static int16_t  arm_deg_last;
static int16_t  det_deg_last;
static uint8_t  payload_last_valid;   /* 0x0A가 유효했는가 */
static uint16_t height_last;

static void set_backlights(uint8_t on)
{
    LCD_SetBacklight(on);
    KEY_SetBacklight(on);
}

/* 패널 출력을 목표 상태로 맞춘다. 픽셀 DMA 중이면 이번 폴은 건너뛰고 다음 폴에 다시 시도.
 * 켜는 방향이면 Display ON이 나간 뒤에 백라이트를 켠다 (빈 화면이 보이지 않게) */
static void panel_apply(void)
{
    if (panel_want_on == panel_is_on)
    {
        return;
    }
    if (!LCD_SetDisplayOn(panel_want_on))
    {
        return;
    }
    panel_is_on = panel_want_on;
    if (panel_is_on)
    {
        set_backlights(1);
    }
}

/* 깨움: Display ON → 백라이트 순. Display OFF가 아직 안 나간 채로 깼으면 백라이트만 켜면 된다 */
static void panel_wake(void)
{
    panel_want_on = 1;
    if (panel_is_on)
    {
        set_backlights(1);
        return;
    }
    panel_apply();
}

void SleepApp_Init(void)
{
    last_activity_tick = HAL_GetTick();
    sleeping           = 0;
    panel_want_on      = 1;   /* LCD_Init이 Display ON + 백라이트 ON 상태로 넘겨준다 */
    panel_is_on        = 1;
    key_last           = 0;
    disp_last_valid    = 0;
    payload_last_valid = 0;
}

/* 이번 폴에 활동이 있었으면 그 이유(로그용 문자열), 없으면 NULL.
 * 활동 유무와 관계없이 "직전 값"은 항상 지금 값으로 갱신한다. */
static const char *detect_activity(uint32_t now)
{
    const char *reason = NULL;

    /* 1) 키: 눌려 있는 동안(장비가 움직이는 중)과 상태가 바뀐 순간 모두 활동 */
    const uint16_t keys = KEY_GetStableMask();
    if (keys != 0U || keys != key_last)
    {
        reason = "key";
    }
    key_last = keys;

    /* 2) EMERGENCY 상태이거나 OP와 두절(DISCONNECTED)인 동안은 절전하지 않는다 — 매 폴을 활동으로 본다 (2026-10-29 결정).
     *    화면과 같은 함수를 쓴다. EMERGENCY는 마지막 수신값이라 두절 중에도 유지된다.
     *    READY로 돌아오거나 다시 연결되면 그때부터 타이머가 다시 센다 */
    if (CAN_App_IsEmergency())
    {
        reason = "emergency";
    }
    else if (!CAN_App_IsConnected())
    {
        reason = "disconnected";
    }

    /* 3) 0x0A: ARM 높이 변화 */
    const BuckPayloadData *p = CAN_App_GetPayloadData();
    const uint8_t p_valid = (p->valid != 0U) &&
                            ((now - p->last_rx_tick) <= CAN_DISPLAY_TIMEOUT_MS);
    if (p_valid)
    {
        if (payload_last_valid && p->payload.Swivel_Height != height_last)
        {
            reason = "arm height";
        }
        height_last = p->payload.Swivel_Height;
    }
    payload_last_valid = p_valid;

    /* 4) 0x02: SID, ARM 회전각, 디텍터 회전각 변화 */
    const BuckDisplayData *d = CAN_App_GetDisplayData();
    const uint8_t d_valid = (d->valid != 0U) &&
                            ((now - d->last_rx_tick) <= CAN_DISPLAY_TIMEOUT_MS);
    if (d_valid)
    {
        if (disp_last_valid)
        {
            if (d->sid_mm != sid_last)
            {
                reason = "sid";
            }
            else if (d->arm_angle_deg != arm_deg_last)
            {
                reason = "arm angle";
            }
            else if (d->det_angle_deg != det_deg_last)
            {
                reason = "det angle";
            }
        }
        sid_last     = d->sid_mm;
        arm_deg_last = d->arm_angle_deg;
        det_deg_last = d->det_angle_deg;
    }
    disp_last_valid = d_valid;

    return reason;
}

void SleepApp_Poll(uint32_t now)
{
    const char *reason = detect_activity(now);

    if (reason != NULL)
    {
        last_activity_tick = now;
        if (sleeping)
        {
            sleeping = 0;
            panel_wake();
            printf("[SLEEP] wake (%s)\r\n", reason);
        }
    }
    else if (!sleeping && (now - last_activity_tick) >= SLEEP_IDLE_TIMEOUT_MS)
    {
        sleeping = 1;
        set_backlights(0);         /* 백라이트 먼저 (즉시 어두워짐) */
        panel_want_on = 0;         /* 그 다음 패널 출력 OFF — 잔상 제거 */
        printf("[SLEEP] off (idle %u s)\r\n", (unsigned)(SLEEP_IDLE_TIMEOUT_MS / 1000U));
    }

    panel_apply();   /* 이번 폴에 바뀐 목표 적용 + DMA 때문에 미뤄진 ON/OFF 재시도 */
}
