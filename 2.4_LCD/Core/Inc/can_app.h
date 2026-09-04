/**
  ******************************************************************************
  * @file    can_app.h
  * @brief   FDCAN1 통신 골격 (BUCKY_KEY_DISPLAY_REV01, TJA1051 트랜시버)
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          현재는 골격 단계: 필터는 전체 수신, 메시지 ID/포맷은 임시값이다.
  *          SU-4100 메인 제어기의 CAN 프로토콜 문서가 확보되면
  *          CAN_TXID_* 정의와 CAN_App_Process()의 수신 분기를 채운다.
  *
  *          사용법:
  *            - main()에서 CAN_App_Init() 1회 호출
  *            - CAN_App_Process()를 메인 컨텍스트(Model::tick)에서 주기 호출
  ******************************************************************************
  */
#ifndef CAN_APP_H
#define CAN_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ==== 임시 메시지 ID — SU-4100 프로토콜 확정 시 교체할 것 ==== */
#define CAN_TXID_KEY_EVENT   0x100U   /* 키 눌림/뗌 보고 (임시) */

typedef struct
{
    uint32_t id;        /* 표준 11비트 ID           */
    uint8_t  dlc;       /* 데이터 길이 0~8          */
    uint8_t  data[8];
} CanRxMsg;

/* 필터/알림 설정 + FDCAN 시작. 성공 시 1, 실패 시 0. */
int CAN_App_Init(void);

/* 클래식 CAN 프레임 송신 (표준 ID, len 0~8). 성공 시 1.
 * TX FIFO가 가득 차 있으면 0을 반환하고 버린다. */
int CAN_App_Send(uint32_t std_id, const uint8_t *data, uint8_t len);

/* 키 이벤트 송신 — 임시 포맷: [key, pressed, mask_L, mask_H] */
int CAN_App_SendKeyEvent(uint8_t key, uint8_t pressed, uint16_t mask);

/* 수신 메시지를 하나 꺼낸다. 있으면 1, 없으면 0. 메인 컨텍스트 전용. */
int CAN_App_PopRx(CanRxMsg *msg);

/* 주기 처리: 활동 LED 소등 타이밍 + 버스오프 자동 복구.
 * 메인 컨텍스트(Model::tick)에서 호출. */
void CAN_App_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* CAN_APP_H */
