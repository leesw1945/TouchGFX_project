/**
  ******************************************************************************
  * @file    app_main.h
  * @brief   애플리케이션 주기 처리 진입점
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          main()의 while(1)에서 MX_TouchGFX_Process()(VSYNC 없으면 즉시
  *          리턴하는 폴링 함수)와 함께 AppMain_Poll()을 매회 호출한다.
  *          인터럽트 → 큐 → 메인 루프 소비 구조의 최상단 함수.
  ******************************************************************************
  */
#ifndef APP_MAIN_H
#define APP_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* main()의 USER CODE 2에서 1회 호출 (CAN 시작 + 키 백라이트 ON) */
void AppMain_Init(void);

/* main()의 while 루프에서 매회 호출:
 * RUN LED 하트비트 + 키 이벤트 소비/CAN 송신 + CAN 수신 처리 */
void AppMain_Poll(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_MAIN_H */
