/**
  ******************************************************************************
  * @file    sleep_app.h
  * @brief   절전 — 일정 시간 활동이 없으면 LCD·키 백라이트를 끄고, 활동이 생기면 다시 켠다
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          활동으로 보는 것 (하나라도 있으면 타이머가 다시 시작되고, 자는 중이면 깬다):
  *            - 버키 키가 눌려 있거나 키 상태가 바뀜
  *            - EMERGENCY 상태인 동안 내내 (절전 금지. READY로 돌아오면 그때부터 다시 센다)
  *            - OP와 두절(DISCONNECTED)인 동안 내내 (절전 금지. 부팅 후 미수신 포함. 다시 연결되면 그때부터 센다)
  *            - OP 값이 바뀜: SID / ARM 회전각 / 디텍터 회전각 (0x02), ARM 높이 (0x0A)
  *
  *          자는 동안에도 CAN 송수신·키 보고·화면 갱신(GRAM 쓰기)은 평소와 같다.
  *          끄는 것: LCD 백라이트(PA1), 키 LED(PA15), 그리고 패널 출력(ST7789 Display OFF — 백라이트만 끄면
  *          주변광에 화면이 비쳐 잔상처럼 보이므로). 깰 때는 Display ON 뒤에 백라이트를 켠다.
  ******************************************************************************
  */
#ifndef SLEEP_APP_H
#define SLEEP_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 활동이 없을 때 백라이트를 끄기까지의 시간.
 * 지금은 동작 확인용 1분. 확인이 끝나면 양산값 10분 = (600U * 1000U) 으로 바꿀 것 */
#define SLEEP_IDLE_TIMEOUT_MS   (60U * 1000U)

/* AppMain_Init에서 1회 호출: 타이머 시작. 백라이트는 켜진 상태(LCD_Init / AppMain_Init이 켬)로 시작한다 */
void SleepApp_Init(void);

/* AppMain_Poll에서 매회 호출 (CAN_App_Process 뒤 — 방금 해석된 수신값으로 비교하기 위해).
 * 활동 감지 -> 타이머 리셋, 자는 중이면 백라이트 ON. 활동 없이 시간 경과 -> 백라이트 OFF */
void SleepApp_Poll(uint32_t now);

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_APP_H */
