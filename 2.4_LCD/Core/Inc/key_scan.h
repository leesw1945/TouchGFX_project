/**
  ******************************************************************************
  * @file    key_scan.h
  * @brief   12키 직접 스캔 + 디바운스 (BUCKY_KEY_DISPLAY_REV01)
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          하드웨어: cKEY0~11 = PB0~PB11 (매트릭스 아님, 키당 GPIO 1개).
  *          액티브 로우 — 보드에 10K 외부 풀업 + 33Ω 직렬 + 바리스터가 있어
  *          MCU는 입력(No pull)으로 읽기만 하면 된다.
  *
  *          사용법:
  *            - KEY_Scan_Tick1ms()를 SysTick_Handler(1ms)에서 호출 (ISR 컨텍스트)
  *            - KEY_PopEvent()를 메인 컨텍스트(Model::tick 등)에서 호출해 소비
  ******************************************************************************
  */
#ifndef KEY_SCAN_H
#define KEY_SCAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define KEY_COUNT           12U
#define KEY_DEBOUNCE_MS     20U   /* 이 시간 동안 상태가 유지돼야 확정 */

typedef struct
{
    uint8_t key;      /* 0~11 (cKEY 번호 = PB 핀 번호) */
    uint8_t pressed;  /* 1 = 눌림, 0 = 뗌 */
} KeyEvent;

/* 1ms마다 호출 (SysTick ISR). PB0~11을 한 번에 읽고 디바운스한다. */
void KEY_Scan_Tick1ms(void);

/* 디바운스 확정된 눌림/뗌 이벤트를 하나 꺼낸다. 있으면 1, 없으면 0.
 * 메인 컨텍스트 전용. */
int KEY_PopEvent(KeyEvent *evt);

/* 현재 확정된 키 상태 비트마스크 (bit0 = cKEY0 ... bit11 = cKEY11, 1 = 눌림) */
uint16_t KEY_GetStableMask(void);

/* 키 백라이트(LED 24개) 켜기/끄기 — PA15 → MP3302 부스트 EN */
void KEY_SetBacklight(uint8_t on);

#ifdef __cplusplus
}
#endif

#endif /* KEY_SCAN_H */
