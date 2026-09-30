/**
  ******************************************************************************
  * @file    can_app.c
  * @brief   버키 키보드 CAN 프로토콜 구현
  *
  *          - 수신: ID 103만 필터 통과 → FIFO0 → 인터럽트에서 링버퍼로
  *          - 해석(메인 컨텍스트): 0x02 표시 / 0x03 높이 / 0x04 상태 저장, 0x06 버전 질의 응답
  *          - 송신 견고성: 앞선 프레임이 ACK를 못 받아 TX 큐에 묵어 있으면(50ms 이상)
  *                 전부 취소하고 호출자가 최신 값으로 다시 넣게 한다.
  *                 → 메인이 늦게 켜지거나 잠깐 끊겼다 붙어도 "지금 키 상태"가 먼저 나간다.
  *          - LED: CAN_TX_LED = 프레임이 실제로 ACK를 받아 송신 완료됐을 때 30ms 펄스
  *                 (큐에 넣기만 한 것은 켜지지 않는다 → 상대가 없으면 안 깜빡임)
  *                 CAN_RX_LED = 수신 30ms 펄스, CAN_ERR_LED = 버스오프 동안 점등
  *          - 버스오프: RM0444 절차대로 CCCR.INIT을 클리어해 자동 복구
  ******************************************************************************
  */
#include "can_app.h"
#include "main.h"
#include "fdcan.h"
#include "key_scan.h"   /* KEY_COUNT */
#include <stdio.h>

#define CAN_RX_QLEN        16U    /* 수신 링버퍼 크기 */
#define CAN_LED_PULSE_MS   30U    /* 활동 LED 점등 시간 */
#define CAN_TX_STALE_MS    50U    /* 큐에 넣은 뒤 이 시간이 지나도 안 나갔으면 "묵은 프레임" */
#define CAN_TX_ALL_BUFFERS (FDCAN_TX_BUFFER0 | FDCAN_TX_BUFFER1 | FDCAN_TX_BUFFER2)

typedef struct
{
    uint32_t id;
    uint8_t  dlc;
    uint8_t  data[8];
} CanRxMsg;

/* HAL의 FDCAN_DLC_BYTES_x 매크로 값이 버전마다 달라서(코드값/바이트수)
 * 테이블로 변환한다 — 양쪽 어느 정의든 안전하게 동작 */
static const uint32_t dlc_code[9] = {
    FDCAN_DLC_BYTES_0, FDCAN_DLC_BYTES_1, FDCAN_DLC_BYTES_2,
    FDCAN_DLC_BYTES_3, FDCAN_DLC_BYTES_4, FDCAN_DLC_BYTES_5,
    FDCAN_DLC_BYTES_6, FDCAN_DLC_BYTES_7, FDCAN_DLC_BYTES_8
};

static uint8_t dlc_to_len(uint32_t code)
{
    for (uint8_t i = 0; i <= 8U; i++)
    {
        if (dlc_code[i] == code)
        {
            return i;
        }
    }
    return 8U;   /* CAN FD 길이 코드(>8바이트)는 8로 잘라서 처리 */
}

/* 수신 링버퍼: 생산자 = FDCAN ISR, 소비자 = 메인 컨텍스트 */
static CanRxMsg          rx_q[CAN_RX_QLEN];
static volatile uint8_t  rx_head = 0;
static volatile uint8_t  rx_tail = 0;

/* 활동 LED 펄스 시각 (0 = 꺼짐 상태) */
static volatile uint32_t tx_led_t0 = 0;
static volatile uint32_t rx_led_t0 = 0;
static volatile uint8_t  bus_off   = 0;

/* 송신 큐 관리 */
static uint32_t          tx_last_add_tick = 0;   /* 마지막으로 큐에 넣은 시각 */
static volatile uint32_t diag_tx_abort    = 0;   /* 묵은 프레임 취소 횟수 (디버거 워치용) */

/* 메인에서 받은 데이터 */
static BuckDisplayData   disp_data;
static BuckHeightData    height_data;
static BuckStateData     state_data;

/* ==== 저수준 송신 ============================================================*/

static int can_send(uint32_t std_id, const uint8_t *data, uint8_t len)
{
    const uint32_t now  = HAL_GetTick();
    const uint32_t free = HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1);

    if (len > 8U)
    {
        len = 8U;
    }

    /* 큐에 아직 안 나간 프레임이 있고, 넣은 지 CAN_TX_STALE_MS가 지났다
     * = 상대가 ACK를 안 해서 재송신만 반복 중. 정상 버스에서는 1ms 안에 나간다.
     * 묵은 프레임을 전부 취소하고 이번 호출은 실패로 돌려, 호출자가 다음 폴에서
     * 최신 값으로 다시 넣게 한다 (취소는 진행 중인 송신 시도가 끝난 뒤 완료됨). */
    if (free < 3U && (now - tx_last_add_tick) > CAN_TX_STALE_MS)
    {
        (void)HAL_FDCAN_AbortTxRequest(&hfdcan1, CAN_TX_ALL_BUFFERS);
        diag_tx_abort++;
        return 0;
    }
    if (free == 0U)
    {
        return 0;   /* 가득 찼지만 아직 신선함(50ms 이내) — 다음 폴에서 재시도 */
    }

    FDCAN_TxHeaderTypeDef h = { 0 };
    h.Identifier          = std_id & 0x7FFU;
    h.IdType              = FDCAN_STANDARD_ID;
    h.TxFrameType         = FDCAN_DATA_FRAME;
    h.DataLength          = dlc_code[len];
    h.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    h.BitRateSwitch       = FDCAN_BRS_OFF;      /* 클래식 CAN */
    h.FDFormat            = FDCAN_CLASSIC_CAN;
    h.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &h, (uint8_t *)data) != HAL_OK)
    {
        return 0;
    }

    tx_last_add_tick = now;
    return 1;
}

/* ==== 초기화 =================================================================*/

int CAN_App_Init(void)
{
    /* 필터: ID 103만 정확히 통과 → FIFO0
     * (메인이 보내는 표시 데이터/버전 질의 모두 ID 103으로 온다)
     * ※ CubeMX FDCAN1 "Std Filters Nbr"가 1 이상이어야 이 필터가 동작한다. */
    FDCAN_FilterTypeDef f = { 0 };
    f.IdType       = FDCAN_STANDARD_ID;
    f.FilterIndex  = 0;
    f.FilterType   = FDCAN_FILTER_MASK;
    f.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    f.FilterID1    = CAN_ID_BUCKY_KEY;
    f.FilterID2    = 0x7FFU;         /* 마스크 전체 비트 비교 = 정확히 일치 */
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &f) != HAL_OK)
    {
        return 0;
    }

    /* 필터에 안 걸린 프레임/리모트 프레임은 전부 거부 */
    if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
                                     FDCAN_REJECT, FDCAN_REJECT,
                                     FDCAN_REJECT_REMOTE,
                                     FDCAN_REJECT_REMOTE) != HAL_OK)
    {
        return 0;
    }

    /* 수신 + 버스오프 + 송신완료 인터럽트 (NVIC의 TIM16_FDCAN_IT0로 들어옴)
     * 송신완료는 TX LED용: 프레임이 실제로 ACK를 받았을 때만 켠다 */
    if (HAL_FDCAN_ActivateNotification(&hfdcan1,
            FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_BUS_OFF | FDCAN_IT_TX_COMPLETE,
            CAN_TX_ALL_BUFFERS) != HAL_OK)
    {
        return 0;
    }

    return (HAL_FDCAN_Start(&hfdcan1) == HAL_OK) ? 1 : 0;
}

/* ==== 프로토콜 송신 ==========================================================*/

/* 물리 키 번호(cKEY0~11) → 프로토콜 비트 번호 변환표.
 *
 * 실측(2026-09-28, LCD를 마주본 기준):
 *   cKEY0~5  = 왼쪽 열 위→아래 (SW2, SW4, SW6, SW8, SW10, SW12)
 *   cKEY6~11 = 오른쪽 열 위→아래 (SW3, SW5, SW7, SW9, SW11, SW13)
 *
 * 메인(SU-6000 Buck_Key.c BuckKeyCodeMap)의 해석:
 *   bit0~5  = 오른쪽 열: ARM_DOWN, ARM_RIGHT_ROT, TUBE_LEFT_SLIDE,
 *                        DET_LEFT_SLIDE, DET_RIGHT_ROT, COLLIMATOR
 *   bit6~11 = 왼쪽 열:   ARM_UP, ARM_LEFT_ROT, TUBE_RIGHT_SLIDE,
 *                        DET_RIGHT_SLIDE, DET_LEFT_ROT, MOV
 *
 * 키캡 배치를 구보드와 같게(왼쪽 열 = ARM_UP ... MOV) 쓰기로 했으므로
 * 왼쪽 열을 bit6~11, 오른쪽 열을 bit0~5로 맞바꿔 보낸다.
 * 키캡 배치가 바뀌면 이 표만 고치면 된다. */
static const uint8_t key_to_proto_bit[KEY_COUNT] = {
    6, 7, 8, 9, 10, 11,     /* cKEY0~5  (왼쪽 열)   → bit6~11 */
    0, 1, 2, 3,  4,  5      /* cKEY6~11 (오른쪽 열) → bit0~5  */
};

static uint16_t key_mask_to_proto(uint16_t key_mask)
{
    uint16_t proto = 0;
    for (uint8_t k = 0; k < KEY_COUNT; k++)
    {
        if (key_mask & (1U << k))
        {
            proto |= (uint16_t)(1U << key_to_proto_bit[k]);
        }
    }
    return proto;
}

int CAN_App_SendKeyValue(uint16_t pressed_mask)
{
    /* 1) 물리 키 비트 → 프로토콜 비트 (좌우 열 변환표 적용)
     * 2) 와이어 규칙: 0 = 눌림 (구보드가 풀업 포트 원시값을 그대로 보냈고,
     *    메인이 ~(값|0xF000)으로 해석한다) → 반전해서 보낸다.
     *    미사용 bit12~15는 반전으로 1이 되어 "안 눌림"으로 읽힌다. */
    uint16_t wire = (uint16_t)~key_mask_to_proto(pressed_mask);

    uint8_t d[3];
    d[0] = CMD_BUCK_KEY_VALUE;
    d[1] = (uint8_t)(wire >> 8);     /* 상위 바이트 먼저 (빅엔디안) */
    d[2] = (uint8_t)wire;

    return can_send(CAN_ID_BUCKY_KEY, d, sizeof(d));
}

/* 버전 질의(0x06) 응답 — 메인의 Version_Receive()가 ID 104 + 0x07을 기다린다 */
static void send_version_info(void)
{
    uint8_t d[4];
    d[0] = CMD_VERSION_INFO;
    d[1] = CAN_ID_BUCKY_KEY;         /* 자기 보드 ID를 실어 보낸다 */
    d[2] = BUCKY_HW_VERSION;
    d[3] = BUCKY_SW_VERSION;

    if (!can_send(CAN_ID_OP_COMMAND, d, sizeof(d)))
    {
        /* 큐가 묵어 있어 실패 — 메인이 다시 질의하면 그때 응답한다 */
        printf("[CAN] version info dropped (tx queue stale)\r\n");
    }
}

/* ==== 수신 해석 (메인 컨텍스트 — printf 사용 가능) ===========================*/

static void process_rx_message(const CanRxMsg *m)
{
    const uint32_t now = HAL_GetTick();

    switch (m->data[0])
    {
    case CMD_BUCK_DISPLAY:           /* 메인 → 보드: LCD 표시 데이터 (300ms) */
        if (m->dlc >= 8U)
        {
            disp_data.unit          = m->data[1];
            disp_data.sid_mm        = (uint16_t)((m->data[2] << 8) | m->data[3]);
            disp_data.arm_angle_deg = (int16_t)((m->data[4] << 8) | m->data[5]);
            disp_data.det_angle_deg = (int16_t)((m->data[6] << 8) | m->data[7]);
            disp_data.last_rx_tick  = now;

            if (!disp_data.valid)
            {
                disp_data.valid = 1;
                printf("[CAN] display data 수신 시작 (unit=%u sid=%umm)\r\n",
                       disp_data.unit, disp_data.sid_mm);
            }
        }
        break;

    case CMD_BUCK_HEIGHT:            /* 메인 → 보드: ARM 높이 (300ms) */
        if (m->dlc >= 3U)
        {
            height_data.height_mm    = (uint16_t)((m->data[1] << 8) | m->data[2]);
            height_data.last_rx_tick = now;
            if (!height_data.valid)
            {
                height_data.valid = 1;
                printf("[CAN] height 수신 시작 (%umm)\r\n", height_data.height_mm);
            }
        }
        break;

    case CMD_BUCK_STATE:             /* 메인 → 보드: 시스템 상태 (변화 시 + 1s) */
        if (m->dlc >= 2U)
        {
            const uint8_t emg = (m->data[1] == BUCK_STATE_EMERGENCY) ? 1U : 0U;
            if (!state_data.valid || state_data.emergency != emg)
            {
                printf("[CAN] state=%s\r\n", emg ? "EMERGENCY" : "READY");
            }
            state_data.emergency    = emg;
            state_data.valid        = 1;
            state_data.last_rx_tick = now;
        }
        break;

    case CMD_QUERY_VERSION:          /* 메인 → 보드: 버전 질의 */
        printf("[CAN] version query -> HW %u / SW %u 응답\r\n",
               BUCKY_HW_VERSION, BUCKY_SW_VERSION);
        send_version_info();
        break;

    default:                         /* 미정의 커맨드 — 프로토콜 확장 시 여기에 추가 */
        printf("[CAN] unknown cmd 0x%02X (dlc=%u)\r\n", m->data[0], m->dlc);
        break;
    }
}

void CAN_App_InjectRx(const uint8_t *data, uint8_t dlc)
{
    CanRxMsg m;
    if (dlc == 0U)
    {
        return;
    }
    if (dlc > 8U)
    {
        dlc = 8U;
    }
    m.id  = CAN_ID_BUCKY_KEY;
    m.dlc = dlc;
    for (uint8_t i = 0; i < 8U; i++)
    {
        m.data[i] = (i < dlc) ? data[i] : 0U;
    }
    process_rx_message(&m);
}

void CAN_App_Process(void)
{
    uint32_t now = HAL_GetTick();

    /* 수신 큐 비우기 */
    CanRxMsg m;
    while (rx_tail != rx_head)
    {
        m = rx_q[rx_tail];
        rx_tail = (uint8_t)((rx_tail + 1U) % CAN_RX_QLEN);
        process_rx_message(&m);
    }

    /* 상태 프레임 타임아웃: 1초 주기 갱신이 끊기면 상태 미상 → READY 취급
     * (구버전 메인처럼 0x04를 아예 안 보내는 경우 EMERGENCY에 갇히지 않게) */
    if (state_data.valid && (now - state_data.last_rx_tick) > CAN_STATE_TIMEOUT_MS)
    {
        state_data.valid = 0;
        if (state_data.emergency)
        {
            printf("[CAN] state frame timeout -> READY\r\n");
        }
        state_data.emergency = 0;
    }

    /* 활동 LED 펄스 종료 */
    if (tx_led_t0 != 0U && (now - tx_led_t0) >= CAN_LED_PULSE_MS)
    {
        HAL_GPIO_WritePin(CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin, GPIO_PIN_SET);
        tx_led_t0 = 0;
    }
    if (rx_led_t0 != 0U && (now - rx_led_t0) >= CAN_LED_PULSE_MS)
    {
        HAL_GPIO_WritePin(CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin, GPIO_PIN_SET);
        rx_led_t0 = 0;
    }

    /* 버스오프 복구: RM0444 — INIT 비트를 클리어하면 컨트롤러가
     * 버스 유휴 129비트 x 11회를 기다린 뒤 자동으로 버스에 복귀한다 */
    if (bus_off)
    {
        FDCAN_ProtocolStatusTypeDef ps;
        HAL_FDCAN_GetProtocolStatus(&hfdcan1, &ps);
        if (ps.BusOff)
        {
            CLEAR_BIT(hfdcan1.Instance->CCCR, FDCAN_CCCR_INIT);
        }
        else
        {
            /* 복구 완료 — ERR LED 소등 */
            HAL_GPIO_WritePin(CAN_ERR_LED_GPIO_Port, CAN_ERR_LED_Pin, GPIO_PIN_SET);
            bus_off = 0;
        }
    }
}

const BuckDisplayData *CAN_App_GetDisplayData(void)
{
    return &disp_data;
}

const BuckHeightData *CAN_App_GetHeightData(void)
{
    return &height_data;
}

const BuckStateData *CAN_App_GetStateData(void)
{
    return &state_data;
}

uint8_t CAN_App_IsEmergency(void)
{
    if (!state_data.valid || !state_data.emergency)
    {
        return 0;
    }
    return ((HAL_GetTick() - state_data.last_rx_tick) <= CAN_STATE_TIMEOUT_MS) ? 1U : 0U;
}

/* ==== HAL 콜백 (ISR 컨텍스트 — printf 금지) ==================================*/

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U)
    {
        return;
    }

    FDCAN_RxHeaderTypeDef rh;
    uint8_t buf[8];

    /* FIFO에 쌓인 것을 전부 꺼내 링버퍼로 옮긴다 */
    while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0U)
    {
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rh, buf) != HAL_OK)
        {
            break;
        }

        uint8_t next = (uint8_t)((rx_head + 1U) % CAN_RX_QLEN);
        if (next != rx_tail)                       /* 가득 차면 버림 */
        {
            CanRxMsg *m = &rx_q[rx_head];
            m->id  = rh.Identifier;
            m->dlc = dlc_to_len(rh.DataLength);
            for (uint8_t i = 0; i < m->dlc; i++)
            {
                m->data[i] = buf[i];
            }
            rx_head = next;
        }
    }

    /* 수신 활동 LED 펄스 시작 */
    HAL_GPIO_WritePin(CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin, GPIO_PIN_RESET);
    rx_led_t0 = HAL_GetTick();
}

void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t BufferIndexes)
{
    (void)hfdcan;
    (void)BufferIndexes;
    /* 프레임이 실제로 ACK를 받아 송신 완료 — TX LED 펄스 (액티브 로우) */
    HAL_GPIO_WritePin(CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin, GPIO_PIN_RESET);
    tx_led_t0 = HAL_GetTick();
}

void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
    (void)hfdcan;
    if (ErrorStatusITs & FDCAN_IT_BUS_OFF)
    {
        bus_off = 1;
        HAL_GPIO_WritePin(CAN_ERR_LED_GPIO_Port, CAN_ERR_LED_Pin, GPIO_PIN_RESET);
    }
}
