/**
  ******************************************************************************
  * @file    app_main.c
  * @brief   애플리케이션 주기 처리 (하트비트 + 키 이벤트 + CAN)
  *
  *          여기의 코드는 전부 메인 컨텍스트(Model::tick)에서 돌기 때문에
  *          printf(USB CDC 콘솔)를 자유롭게 쓸 수 있다.
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

#define HEARTBEAT_PERIOD_MS  500U

void AppMain_Init(void)
{
    if (CAN_App_Init())
    {
        printf("[2.4_LCD] CAN start OK (500 kbit/s)\r\n");
    }
    else
    {
        printf("[2.4_LCD] CAN start FAILED\r\n");
    }

    KEY_SetBacklight(1);
}

void AppMain_Poll(void)
{
    /* ---- RUN LED 하트비트 (액티브 로우, 500ms 토글) ---- */
    static uint32_t hb_t0 = 0;
    uint32_t now = HAL_GetTick();
    if ((now - hb_t0) >= HEARTBEAT_PERIOD_MS)
    {
        hb_t0 = now;
        HAL_GPIO_TogglePin(RUN_LED_GPIO_Port, RUN_LED_Pin);
    }

    /* ---- 키 이벤트 소비: 로그 + CAN 보고 ---- */
    KeyEvent ke;
    while (KEY_PopEvent(&ke))
    {
        printf("[KEY] %u %s (mask=0x%03X)\r\n",
               ke.key, ke.pressed ? "DOWN" : "UP", KEY_GetStableMask());
        CAN_App_SendKeyEvent(ke.key, ke.pressed, KEY_GetStableMask());

        /* TODO: SU-4100 프로토콜 확정 후 Model을 통해 UI에도 전달 */
    }

    /* ---- CAN 주기 처리 (LED 소등, 버스오프 복구) ---- */
    CAN_App_Process();

    /* ---- CAN 수신 처리: 지금은 브링업용 전체 로그 ----
     * TODO: 프로토콜 확정 후 ID별 분기(Emergency 표시 명령 등)로 교체.
     * 버스 트래픽이 많은 장비에 붙이면 로그가 넘치니 그때는 지울 것. */
    CanRxMsg rx;
    while (CAN_App_PopRx(&rx))
    {
        printf("[CAN RX] id=0x%03lX dlc=%u data=", (unsigned long)rx.id, rx.dlc);
        for (uint8_t i = 0; i < rx.dlc; i++)
        {
            printf("%02X ", rx.data[i]);
        }
        printf("\r\n");
    }
}
