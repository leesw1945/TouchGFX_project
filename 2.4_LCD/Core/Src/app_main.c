/**
  ******************************************************************************
  * @file    app_main.c
  * @brief   애플리케이션 주기 처리 (하트비트 + 키 보고 + CAN)
  *
  *          여기의 코드는 전부 메인 컨텍스트(Model::tick)에서 돌기 때문에
  *          printf(USB CDC 콘솔)를 자유롭게 쓸 수 있다.
  *
  *          키 보고 정책 (SU-6000 프로토콜):
  *          - 키가 변하면 즉시 CMD_BUCK_KEY_VALUE 송신
  *          - 변화가 없어도 300ms마다 재송신
  *            (메인이 ID 103 수신 1.5초 두절 시 통신 에러로 처리하므로)
  *
  *          참고: RUN LED가 깜빡인다 = TE 인터럽트와 TouchGFX 프레임 루프가
  *          살아 있다는 뜻이다. LCD/TE에 문제가 생기면 하트비트도 멈추므로
  *          그 자체가 디스플레이 파이프라인의 생사 표시가 된다.
  ******************************************************************************
  */
#include "app_main.h"
#include "main.h"
#include "key_scan.h"
#include "can_app.h"
#include <stdio.h>

#define HEARTBEAT_PERIOD_MS   500U
#define KEY_REPORT_PERIOD_MS  300U   /* 키 값 주기 재송신 (얼라이브 겸용) */

void AppMain_Init(void)
{
    if (CAN_App_Init())
    {
        printf("[2.4_LCD] CAN start OK (500 kbit/s, ID 0x%02X)\r\n", CAN_ID_BUCKY_KEY);
    }
    else
    {
        printf("[2.4_LCD] CAN start FAILED\r\n");
    }

    KEY_SetBacklight(1);
}

void AppMain_Poll(void)
{
    uint32_t now = HAL_GetTick();

    /* ---- RUN LED 하트비트 (액티브 로우, 500ms 토글) ---- */
    static uint32_t hb_t0 = 0;
    if ((now - hb_t0) >= HEARTBEAT_PERIOD_MS)
    {
        hb_t0 = now;
        HAL_GPIO_TogglePin(RUN_LED_GPIO_Port, RUN_LED_Pin);
    }

    /* ---- 키 보고: 변화 즉시 + 300ms 주기 재송신 ---- */
    static uint32_t key_tx_t0 = 0;
    KeyEvent ke;
    uint8_t  key_changed = 0;

    while (KEY_PopEvent(&ke))
    {
        printf("[KEY] %u %s (mask=0x%03X)\r\n",
               ke.key, ke.pressed ? "DOWN" : "UP", KEY_GetStableMask());
        key_changed = 1;

        /* TODO: Emergency 등 UI 연동은 프로토콜 협의 후 Model을 통해 연결 */
    }

    if (key_changed || (now - key_tx_t0) >= KEY_REPORT_PERIOD_MS)
    {
        key_tx_t0 = now;
        CAN_App_SendKeyValue(KEY_GetStableMask());
    }

    /* ---- CAN 주기 처리 (수신 해석, LED 소등, 버스오프 복구) ---- */
    CAN_App_Process();

    /* ---- 표시 데이터 변화 로그 (브링업용, 최대 1초에 1회) ----
     * TODO: UI 연결 시 이 로그 대신 Model이 CAN_App_GetDisplayData()를 읽어
     *       화면 위젯을 갱신한다. */
    static BuckyDisplayData disp_last;
    static uint32_t         disp_log_t0 = 0;
    const BuckyDisplayData *d = CAN_App_GetDisplayData();

    if (d->valid && (now - disp_log_t0) >= 1000U &&
        (d->unit          != disp_last.unit ||
         d->sid_mm        != disp_last.sid_mm ||
         d->arm_angle_deg != disp_last.arm_angle_deg ||
         d->det_angle_deg != disp_last.det_angle_deg))
    {
        disp_log_t0 = now;
        disp_last   = *d;
        printf("[DISP] unit=%s SID=%umm ARM=%ddeg DET=%ddeg\r\n",
               d->unit ? "Inch" : "Cm", d->sid_mm,
               d->arm_angle_deg, d->det_angle_deg);
    }
}
