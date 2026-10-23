/**
  ******************************************************************************
  * @file    key_scan.h
  * @brief   12키 직접 스캔 — 10ms 주기, 디바운스 없음 (BUCKY_KEY_DISPLAY_REV01)
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          하드웨어: cKEY0~11 = PB0~PB11 (매트릭스 아님, 키당 GPIO 1개).
  *          액티브 로우 — 보드에 10K 외부 풀업 + 33Ω 직렬 + 바리스터가 있어
  *          MCU는 입력(No pull)으로 읽기만 하면 된다.
  *
  *          OP(메인) 보드 요구사항(2026-10-23): 키 포트를 10ms 주기로 읽어 현재 값을
  *          그대로 보낸다. 채터링(디바운스)·짧게/길게 누름 판정은 전부 OP가 한다.
  *
  *          사용법:
  *            - KEY_Scan_Tick1ms()를 SysTick_Handler(1ms)에서 호출 (ISR 컨텍스트).
  *              내부에서 10ms마다 한 번 포트를 읽는다.
  *            - KEY_PopEvent()는 로그용 이벤트(상태가 바뀐 키)를 메인 컨텍스트에서 꺼낸다
  ******************************************************************************
  */
#ifndef KEY_SCAN_H
#define KEY_SCAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define KEY_COUNT           12U
#define KEY_SCAN_PERIOD_MS  10U   /* 키 포트 읽기 주기 (OP 요구사항) */

typedef struct
{
    uint8_t key;      /* 0~11 (cKEY 번호 = PB 핀 번호) */
    uint8_t pressed;  /* 1 = 눌림, 0 = 뗌 */
} KeyEvent;

/* 1ms마다 호출 (SysTick ISR). KEY_SCAN_PERIOD_MS마다 PB0~11을 한 번에 읽는다. 디바운스 없음. */
void KEY_Scan_Tick1ms(void);

/* 상태가 바뀐 키의 눌림/뗌 이벤트를 하나 꺼낸다(로그용). 있으면 1, 없으면 0.
 * 메인 컨텍스트 전용. */
int KEY_PopEvent(KeyEvent *evt);

/* 가장 최근 10ms 샘플의 키 상태 비트마스크 (bit0 = cKEY0 ... bit11 = cKEY11, 1 = 눌림) */
uint16_t KEY_GetStableMask(void);

/* 키 백라이트(LED 24개) 켜기/끄기 — PA15 → MP3302 부스트 EN */
void KEY_SetBacklight(uint8_t on);

#ifdef __cplusplus
}
#endif

#endif /* KEY_SCAN_H */
