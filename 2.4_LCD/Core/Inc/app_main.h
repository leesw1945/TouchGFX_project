/**
  ******************************************************************************
  * @file    app_main.h
  * @brief   애플리케이션 주기 처리 진입점
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          MX_TouchGFX_Process()가 내부 무한 루프라서 main()의 while(1)
  *          아래 코드는 실행되지 않는다. 그래서 주기 작업은
  *          TouchGFX Model::tick()(매 프레임, 약 76Hz, 메인 컨텍스트)에서
  *          AppMain_Poll()을 불러 처리한다.
  ******************************************************************************
  */
#ifndef APP_MAIN_H
#define APP_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* main()의 USER CODE 2에서 1회 호출 (CAN 시작 + 키 백라이트 ON) */
void AppMain_Init(void);

/* Model::tick()에서 매 프레임 호출:
 * RUN LED 하트비트 + 키 이벤트 소비/CAN 송신 + CAN 수신 처리 */
void AppMain_Poll(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_MAIN_H */
