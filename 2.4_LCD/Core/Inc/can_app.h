/**
  ******************************************************************************
  * @file    can_app.h
  * @brief   버키 키보드 CAN 프로토콜 (BUCKY_KEY_DISPLAY_REV01, TJA1051)
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          프로토콜 출처: SU-6000 SDK Rev14 메인 OP 펌웨어 분석
  *          (Buck_Key.c / Sensor_Collect.c / CAN_Driver.h / Version.c)
  *          0x03, 0x04는 SU-4100에서 새로 제안한 커맨드 (메인 담당과 협의 중).
  *
  *          규칙 요약:
  *          - 클래식 CAN 500 kbit/s, 11비트 표준 ID, 데이터 최대 8바이트
  *          - ID 103(0x67) 하나를 메인↔보드 양방향 공용, Data[0] 커맨드 바이트로 구분
  *          - 두 바이트 숫자는 상위 바이트 먼저(빅엔디안), 각도는 부호 있는 16비트
  *          - 메인은 ID 103 수신이 1.5초 없으면 통신 에러 처리
  *            → 보드는 키 변화가 없어도 300ms마다 키 값을 재송신(얼라이브 겸용)
  *          - 보드는 0x02/0x03이 1.5초 없으면 통신 두절로 보고 화면을 대시로 바꾼다
  *          - 0x04 EMERGENCY 수신 중에는 물리 키와 무관하게 "모두 안 눌림"을 보낸다
  *
  *          ID 103 커맨드 목록 (Data[0]):
  *            0x01 CMD_BUCK_KEY_VALUE  보드→메인  DLC3  [1..2] 키 16비트(0=눌림)
  *            0x02 CMD_BUCK_DISPLAY    메인→보드  DLC8  [1] 단위 [2..3] SID mm [4..5] ARM각 [6..7] DET각 (300ms)
  *            0x03 CMD_BUCK_HEIGHT     메인→보드  DLC8  [1..2] ARM 높이 mm, [3..7] 예약 0            (300ms)
  *            0x04 CMD_BUCK_STATE      메인→보드  DLC8  [1] 0 READY / 1 EMERGENCY, [2..7] 예약 0    (변화 시 + 1s)
  *            0x06 CMD_QUERY_VERSION   메인→보드  DLC1
  *            0x07 CMD_VERSION_INFO    보드→메인  DLC4  [1] 103 [2] HW [3] SW  ※ 이것만 ID 104로 송신
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
#define CMD_BUCK_KEY_VALUE   0x01U   /* 보드→메인: 키 상태 16비트                    */
#define CMD_BUCK_DISPLAY     0x02U   /* 메인→보드: LCD 표시 데이터(300ms)            */
#define CMD_BUCK_HEIGHT      0x03U   /* 메인→보드: ARM 높이 mm(300ms)     [신규 제안] */
#define CMD_BUCK_STATE       0x04U   /* 메인→보드: 시스템 상태(변화 시+1s) [신규 제안] */
#define CMD_QUERY_VERSION    0x06U   /* 메인→보드: 버전 질의                         */
#define CMD_VERSION_INFO     0x07U   /* 보드→메인: 버전 응답 (ID 104로)              */

/* 0x04 상태 값 */
#define BUCK_STATE_READY     0U
#define BUCK_STATE_EMERGENCY 1U

/* ==== 이 보드의 버전 (버전 응답에 실림 — 릴리스 시 갱신할 것) ==== */
#define BUCKY_HW_VERSION     1U      /* REV01 */
#define BUCKY_SW_VERSION     1U

/* ==== 수신 데이터 유효 기준 ==== */
#define CAN_DISPLAY_TIMEOUT_MS  1500U  /* 0x02/0x03이 이보다 오래 없으면 통신 두절 → 화면 대시 */
#define CAN_STATE_TIMEOUT_MS    3000U  /* 0x04(1초 주기)가 3초 없으면 상태 미상 → READY 취급    */

/* 메인이 300ms마다 보내주는 LCD 표시 데이터 (CMD_BUCK_DISPLAY 파싱 결과) */
typedef struct
{
    uint8_t  valid;          /* 1 = 최소 한 번은 수신함                   */
    uint8_t  unit;           /* 0 = Cm, 1 = Inch                          */
    uint16_t sid_mm;         /* SID 거리 [mm] (40인치 = 1016)             */
    int16_t  arm_angle_deg;  /* ARM 회전각 [도] (-30 ~ 120)               */
    int16_t  det_angle_deg;  /* 디텍터 회전각 [도] (-45 ~ 45)             */
    uint32_t last_rx_tick;   /* 마지막 수신 시각(HAL_GetTick) — 두절 판단용 */
} BuckDisplayData;

/* ARM 높이 (CMD_BUCK_HEIGHT 파싱 결과) */
typedef struct
{
    uint8_t  valid;
    uint16_t height_mm;      /* ARM UP/DOWN 높이 [mm] */
    uint32_t last_rx_tick;
} BuckHeightData;

/* 시스템 상태 (CMD_BUCK_STATE 파싱 결과) */
typedef struct
{
    uint8_t  valid;
    uint8_t  emergency;      /* 1 = EMERGENCY */
    uint32_t last_rx_tick;
} BuckStateData;

/* 필터/알림 설정 + FDCAN 시작. 성공 시 1, 실패 시 0. */
int CAN_App_Init(void);

/* 키 상태 송신 (CMD_BUCK_KEY_VALUE).
 * pressed_mask: 물리 키 bit0~11 (cKEY0~11), 1 = 눌림 (key_scan의 KEY_GetStableMask() 그대로).
 * 물리 키 → 프로토콜 비트 변환(좌우 열 맞바꿈, can_app.c의 key_to_proto_bit 표)과
 * 와이어 규칙(0 = 눌림, 액티브 로우)으로의 반전은 이 함수가 처리한다.
 * 성공 시 1. 앞선 프레임이 ACK를 못 받아 묵어 있으면 그것을 취소하고 0을 돌려주므로
 * 호출자는 다음 폴에서 최신 값으로 다시 호출해야 한다. */
int CAN_App_SendKeyValue(uint16_t pressed_mask);

/* 주기 처리: 수신 메시지 해석(표시/높이/상태 저장, 버전 질의 응답) +
 * 상태 타임아웃 + 활동 LED 소등 + 버스오프 자동 복구. 메인 루프에서 호출. */
void CAN_App_Process(void);

/* 마지막으로 수신한 데이터 (UI에서 읽어감. valid==0이면 아직 미수신.
 * 두절 판단은 last_rx_tick과 CAN_DISPLAY_TIMEOUT_MS로 읽는 쪽에서 한다) */
const BuckDisplayData *CAN_App_GetDisplayData(void);
const BuckHeightData  *CAN_App_GetHeightData(void);
const BuckStateData   *CAN_App_GetStateData(void);

/* EMERGENCY 여부 (0x04 수신값 + CAN_STATE_TIMEOUT_MS 반영). 1 = 비상 */
uint8_t CAN_App_IsEmergency(void);

/* 진단: 묵은 송신 프레임을 취소한 누적 횟수 (정상 버스에서는 0에 머문다) */
uint32_t CAN_App_GetTxAbortCount(void);

/* 테스트용: 콘솔 등에서 "수신 프레임"을 주입한다. CAN으로 받은 것과 같은 해석 코드를 탄다.
 * data[0] = 커맨드 바이트, dlc = 바이트 수(1~8). 메인 컨텍스트에서만 호출. */
void CAN_App_InjectRx(const uint8_t *data, uint8_t dlc);

#ifdef __cplusplus
}
#endif

#endif /* CAN_APP_H */
