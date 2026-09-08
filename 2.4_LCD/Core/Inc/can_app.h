/**
  ******************************************************************************
  * @file    can_app.h
  * @brief   버키 키보드 CAN 프로토콜 (BUCKY_KEY_DISPLAY_REV01, TJA1051)
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          프로토콜 출처: SU-6000 SDK Rev14 메인 OP 펌웨어 분석
  *          (Buck_Key.c / Sensor_Collect.c / CAN_Driver.h / Version.c)
  *
  *          규칙 요약:
  *          - 클래식 CAN 500 kbit/s, 11비트 표준 ID
  *          - ID 103(0x67) 하나를 메인↔보드 양방향 공용, Data[0]로 구분
  *          - 메인은 ID 103 수신이 1.5초 없으면 통신 에러 처리
  *            → 보드는 키 변화가 없어도 주기적으로 키 값을 재송신해야 함
  *          - Emergency 표시 명령은 아직 없음 — 메인 담당과 협의 후 추가 예정
  ******************************************************************************
  */
#ifndef CAN_APP_H
#define CAN_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ==== CAN ID (SU-6000 SDK CAN_Driver.h의 SENSOR_BOARD_CAN_ID enum) ==== */
#define CAN_ID_BUCKY_KEY     0x67U   /* 103: 이 보드 (양방향 공용)        */
#define CAN_ID_OP_COMMAND    0x68U   /* 104: 버전 응답을 이 ID로 송신     */

/* ==== 커맨드 바이트 (Data[0]) ==== */
#define CMD_BUCK_KEY_VALUE   0x01U   /* 보드→메인: 키 상태 16비트         */
#define CMD_BUCK_DISPLAY     0x02U   /* 메인→보드: LCD 표시 데이터(300ms) */
#define CMD_QUERY_VERSION    0x06U   /* 메인→보드: 버전 질의              */
#define CMD_VERSION_INFO     0x07U   /* 보드→메인: 버전 응답 (ID 104로)   */

/* ==== 이 보드의 버전 (버전 응답에 실림 — 릴리스 시 갱신할 것) ==== */
#define BUCKY_HW_VERSION     1U      /* REV01 */
#define BUCKY_SW_VERSION     1U

/* 메인이 300ms마다 보내주는 LCD 표시 데이터 (CMD_BUCK_DISPLAY 파싱 결과) */
typedef struct
{
    uint8_t  valid;          /* 1 = 최소 한 번은 수신함                   */
    uint8_t  unit;           /* 0 = Cm, 1 = Inch                          */
    uint16_t sid_mm;         /* SID 거리 [mm] (40인치 = 1016)             */
    int16_t  arm_angle_deg;  /* ARM 회전각 [도]                           */
    int16_t  det_angle_deg;  /* 디텍터 회전각 [도]                        */
    uint32_t last_rx_tick;   /* 마지막 수신 시각(HAL_GetTick) — 두절 판단용 */
} BuckyDisplayData;

/* 필터/알림 설정 + FDCAN 시작. 성공 시 1, 실패 시 0. */
int CAN_App_Init(void);

/* 키 상태 송신 (CMD_BUCK_KEY_VALUE).
 * pressed_mask: bit0~11, 1 = 눌림 (key_scan의 KEY_GetStableMask() 그대로).
 * 와이어 규칙(0 = 눌림, 액티브 로우)으로의 반전은 이 함수가 처리한다.
 * 성공 시 1, TX FIFO가 가득이면 0. */
int CAN_App_SendKeyValue(uint16_t pressed_mask);

/* 주기 처리: 수신 메시지 해석(표시 데이터 저장, 버전 질의 응답) +
 * 활동 LED 소등 + 버스오프 자동 복구. 메인 컨텍스트(Model::tick)에서 호출. */
void CAN_App_Process(void);

/* 마지막으로 수신한 표시 데이터 (UI에서 읽어감. valid==0이면 아직 미수신) */
const BuckyDisplayData *CAN_App_GetDisplayData(void);

#ifdef __cplusplus
}
#endif

#endif /* CAN_APP_H */
