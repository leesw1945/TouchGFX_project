/**
  ******************************************************************************
  * @file    key_scan.c
  * @brief   12키 직접 스캔 + 디바운스 구현
  *
  *          cKEY0~11이 전부 포트 B의 bit0~11에 연속 배치되어 있어서
  *          GPIOB->IDR 한 번 읽기로 12키 전체를 스캔한다.
  ******************************************************************************
  */
#include "key_scan.h"
#include "main.h"

#define KEY_MASK        0x0FFFU               /* PB0~PB11 */
#define KEY_EVT_QLEN    16U                   /* 이벤트 링버퍼 크기 (2의 거듭제곱) */

static volatile uint16_t stable_mask = 0;     /* 디바운스 확정 상태 (1 = 눌림)     */
static uint16_t          raw_last    = 0;     /* 직전 1ms 샘플                      */
static uint16_t          raw_count   = 0;     /* 같은 샘플이 연속된 횟수(ms)        */

/* 이벤트 링버퍼: 생산자 = SysTick ISR, 소비자 = 메인 컨텍스트 */
static KeyEvent          evt_q[KEY_EVT_QLEN];
static volatile uint8_t  evt_head = 0;        /* ISR에서만 갱신   */
static volatile uint8_t  evt_tail = 0;        /* 메인에서만 갱신  */

static void push_event(uint8_t key, uint8_t pressed)
{
    uint8_t next = (uint8_t)((evt_head + 1U) % KEY_EVT_QLEN);
    if (next == evt_tail)
    {
        return;                               /* 가득 참: 이벤트 버림 */
    }
    evt_q[evt_head].key     = key;
    evt_q[evt_head].pressed = pressed;
    evt_head = next;
}

void KEY_Scan_Tick1ms(void)
{
    /* 액티브 로우이므로 반전해서 "1 = 눌림"으로 만든다 */
    uint16_t raw = (uint16_t)(~GPIOB->IDR) & KEY_MASK;

    if (raw != raw_last)
    {
        /* 상태가 흔들리는 중(바운스) — 카운터 리셋 */
        raw_last  = raw;
        raw_count = 0;
        return;
    }

    if (raw_count < KEY_DEBOUNCE_MS)
    {
        raw_count++;
        if (raw_count == KEY_DEBOUNCE_MS && raw != stable_mask)
        {
            /* KEY_DEBOUNCE_MS 동안 유지됨 — 상태 확정, 바뀐 키마다 이벤트 발행 */
            uint16_t changed = (uint16_t)(raw ^ stable_mask);
            stable_mask = raw;

            for (uint8_t k = 0; k < KEY_COUNT; k++)
            {
                if (changed & (1U << k))
                {
                    push_event(k, (raw >> k) & 1U);
                }
            }
        }
    }
}

int KEY_PopEvent(KeyEvent *evt)
{
    if (evt_tail == evt_head)
    {
        return 0;
    }
    *evt = evt_q[evt_tail];
    evt_tail = (uint8_t)((evt_tail + 1U) % KEY_EVT_QLEN);
    return 1;
}

uint16_t KEY_GetStableMask(void)
{
    return stable_mask;
}

void KEY_SetBacklight(uint8_t on)
{
    HAL_GPIO_WritePin(KEY_BL_GPIO_Port, KEY_BL_Pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
