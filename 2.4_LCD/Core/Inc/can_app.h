/**
  ******************************************************************************
  * @file    can_app.h
  * @brief   버키 키보드 CAN 프로토콜 (BUCKY_KEY_DISPLAY_REV01, TJA1051)
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          프로토콜 출처: SU-6000 SDK Rev14 메인 OP 펌웨어 분석
  *          (Buck_Key.c / Sensor_Collect.c / CAN_Driver.h / Version.c)
  *          0x0A(10)는 OP 보드 담당이 정의한 신규 커맨드 (2026-10-23 참고 코드 기준).
  *          이전에 제안했던 0x03(높이)/0x04(상태)는 0x0A 하나로 대체되어 삭제했다.
  *
  *          규칙 요약:
  *          - 클래식 CAN 500 kbit/s, 11비트 표준 ID, 데이터 최대 8바이트
  *          - ID 103(0x67) 하나를 메인↔보드 양방향 공용, Data[0] 커맨드 바이트로 구분
  *          - 0x01/0x02의 두 바이트 숫자는 상위 바이트 먼저(빅엔디안), 각도는 부호 있는 16비트
  *            ※ 0x0A의 높이는 OP 구조체 그대로 리틀엔디안(LSB 먼저) — 커맨드마다 다르니 주의
  *          - 키 포트는 10ms 주기로 읽고 디바운스 없이 보낸다(채터링 판정은 OP 담당)
  *          - 키 상태가 직전 송신값과 다르면 즉시, 같으면 500ms 주기로 송신
  *            (메인은 ID 103 수신이 1.5초 없으면 통신 에러 처리 → 얼라이브 겸용)
  *          - 보드는 0x02/0x0A가 1.5초 없으면 통신 두절로 보고 그 값을 대시로 바꾼다.
  *            둘 다 없으면(부팅 후 미수신 포함) 상단 바를 DISCONNECTED(주황)로 바꾼다
  *          - 0x0A의 Emergency가 1(EMG_EMERGENCY)인 동안에는 물리 키와 무관하게 "모두 안 눌림"을 보낸다
  *            (0x0A가 두절되어도 마지막 값을 유지 — EMERGENCY 중 두절이면 OP가 READY를 다시 보낼 때까지 차단)
  *
  *          ID 103 커맨드 목록 (Data[0]):
  *            0x01 CMD_BUCK_KEY_VALUE    보드→메인  DLC3  [1..2] 키 16비트(0=눌림, bit12~15=1)
  *            0x02 CMD_BUCK_DISPLAY      메인→보드  DLC8  [1] 단위 [2..3] SID mm [4..5] ARM각 [6..7] DET각 (300ms)
  *            0x0A CMD_BUCK_KEY_PAYLOAD  메인→보드  DLC5  [1] 단위 [2] Emergency [3..4] ARM 높이 mm(LSB, MSB) (300ms)
  *            0x06 CMD_QUERY_VERSION     메인→보드  DLC1
  *            0x07 CMD_VERSION_INFO      보드→메인  DLC4  [1] 103 [2] HW [3] SW  ※ 이것만 ID 104로 송신
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
#define CMD_BUCK_KEY_VALUE    0x01U  /* 보드→메인: 키 상태 16비트                          */
#define CMD_BUCK_DISPLAY      0x02U  /* 메인→보드: LCD 표시 데이터(300ms)                  */
#define CMD_QUERY_VERSION     0x06U  /* 메인→보드: 버전 질의                               */
#define CMD_VERSION_INFO      0x07U  /* 보드→메인: 버전 응답 (ID 104로)                    */
#define CMD_BUCK_KEY_PAYLOAD  10U    /* 0x0A 메인→보드: 단위 + Emergency + ARM 높이 (OP 보드 정의) */

/* ==== 이 보드의 버전 (버전 응답과 부팅 팝업에 실림 — 릴리스 시 갱신할 것) ==== */
#define BUCKY_HW_VERSION     1U      /* REV01 */
#define BUCKY_SW_VERSION     1U      /* 1: 출하 기준 번호를 1부터 다시 시작 (2026-10-28).
                                        CAN 0x07 응답, 콘솔 로그, 부팅 버전 팝업(MainView)이 모두 이 값을 쓴다 */

/* ==== 수신 데이터 유효 기준 ====
 * 0x02, 0x0A 모두 OP가 300ms 주기로 송신한다(OP 담당 확인 2026-10-23) → 5회 연속 놓치면 두절로 본다. */
#define CAN_DISPLAY_TIMEOUT_MS  1500U  /* 0x02/0x0A가 이보다 오래 없으면 통신 두절 → 값은 화면 대시. READY/EMERGENCY(화면·키 차단)는 마지막 값 유지 */

/* ==== OP 보드와 공유하는 열거형 — SU-6000 SDK 정의 그대로, 값을 바꾸면 안 됨 ==== */

/* UART_Protocol.h: typedef enum { UNIT_Cm, UNIT_Inch } UNIT_Type; */
typedef enum
{
    UNIT_Cm   = 0,
    UNIT_Inch = 1
} UNIT_Type;

/* 와이어 값 규칙(2026-10-26 확정): 0 = READY, 1 = EMERGENCY.
 * ※ SDK USER_Type_Define.H 79행의 typedef enum { EMG_ON, EMG_OFF }(EMG_ON = 0)와는 반대이므로
 *    SDK 이름을 쓰지 않고 값이 드러나는 이름으로 정의한다. OP가 보내는 값과 맞는지 통합 시험에서 확인할 것. */
typedef enum
{
    EMG_READY     = 0,
    EMG_EMERGENCY = 1
} EMERGENCY_STATE;

/* ==== 0x0A 수신 페이로드 — OP 보드 담당이 준 참고 코드와 바이트 배치가 1:1 (2026-10-23) ====
 * 필드 순서·크기·엔디안을 OP 쪽과 똑같이 유지해야 한다. 바꾸면 안 됨.
 * 이 툴체인(arm-none-eabi-gcc 13.3, 기본 -fshort-enums)에서 enum 멤버는 1바이트 → 총 4바이트.
 * can_app.c / Model.cpp에서 4바이트인지 빌드 시 검사한다. */
typedef struct __attribute__((packed))
{
    UNIT_Type       Unit;           /* 1바이트: UNIT_Cm(0) / UNIT_Inch(1)              */
    EMERGENCY_STATE Emergency;      /* 1바이트: 0 = READY / 1 = EMERGENCY               */
    uint16_t        Swivel_Height;  /* 2바이트: ARM 높이 mm, 리틀엔디안(LSB, MSB 순)    */
} OP_To_Bucky_Key_Payload_t;

typedef union
{
    uint8_t                   Bytes[sizeof(OP_To_Bucky_Key_Payload_t)];
    OP_To_Bucky_Key_Payload_t Payload;
} OP_To_Bucky_Key_Union_t;

/* 0x0A 프레임 길이 = 커맨드(1) + 페이로드(4). 다르면 참고 코드와 같이 무시한다 */
#define CMD_BUCK_KEY_PAYLOAD_DLC  5U

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

/* OP 보드에서 받은 0x0A 페이로드 (참고 코드의 OP_Rx_Payload에 해당) */
typedef struct
{
    uint8_t                   valid;         /* 1 = 수신 중 (타임아웃이면 0으로 돌아감)                 */
    OP_To_Bucky_Key_Payload_t payload;       /* 마지막 수신값 그대로 (Unit / Emergency / Swivel_Height) */
    uint32_t                  last_rx_tick;  /* 마지막 수신 시각 — 두절 판단용                           */
} BuckPayloadData;

/* 필터/알림 설정 + FDCAN 시작. 성공 시 1, 실패 시 0. */
int CAN_App_Init(void);

/* 키 상태 송신 (CMD_BUCK_KEY_VALUE).
 * pressed_mask: 물리 키 bit0~11 (cKEY0~11), 1 = 눌림 (key_scan의 KEY_GetStableMask() 그대로).
 * 물리 키 → 프로토콜 비트 변환(좌우 열 맞바꿈, can_app.c의 key_to_proto_bit 표)과
 * 와이어 규칙(0 = 눌림, 액티브 로우)으로의 반전은 이 함수가 처리한다.
 * 성공 시 1. 앞선 프레임이 ACK를 못 받아 묵어 있으면 그것을 취소하고 0을 돌려주므로
 * 호출자는 다음 폴에서 최신 값으로 다시 호출해야 한다. */
int CAN_App_SendKeyValue(uint16_t pressed_mask);

/* 주기 처리: 수신 메시지 해석(표시/페이로드 저장, 버전 질의 응답) +
 * 0x0A 타임아웃 + 활동 LED 소등 + 버스오프 자동 복구. 메인 루프에서 호출. */
void CAN_App_Process(void);

/* 마지막으로 수신한 데이터 (UI에서 읽어감. valid==0이면 미수신.
 * 두절 판단은 last_rx_tick과 CAN_DISPLAY_TIMEOUT_MS로 읽는 쪽에서 한다) */
const BuckDisplayData *CAN_App_GetDisplayData(void);
const BuckPayloadData *CAN_App_GetPayloadData(void);

/* EMERGENCY 여부: 마지막으로 받은 0x0A의 Emergency == 1(EMG_EMERGENCY)이면 1. 두절과 무관하게 유지된다(2026-10-29).
 * 화면 상태 표시(Model)와 키 차단(app_main)이 모두 이 함수 하나를 쓴다 */
uint8_t CAN_App_IsEmergency(void);

/* OP 연결 여부: 0x02 또는 0x0A를 CAN_DISPLAY_TIMEOUT_MS 안에 받았으면 1. 부팅 후 한 번도 못 받았거나 둘 다 끊기면 0.
 * 상단 바 DISCONNECTED 표시(Model)가 쓴다. 키 차단은 이것과 무관하게 Emergency만 본다 */
uint8_t CAN_App_IsConnected(void);

/* 진단: 묵은 송신 프레임을 취소한 누적 횟수 (정상 버스에서는 0에 머문다) */
uint32_t CAN_App_GetTxAbortCount(void);

/* 테스트용: 콘솔 등에서 "수신 프레임"을 주입한다. CAN으로 받은 것과 같은 해석 코드를 탄다.
 * data[0] = 커맨드 바이트, dlc = 바이트 수(1~8). 메인 컨텍스트에서만 호출. */
void CAN_App_InjectRx(const uint8_t *data, uint8_t dlc);

#ifdef __cplusplus
}
#endif

#endif /* CAN_APP_H */
