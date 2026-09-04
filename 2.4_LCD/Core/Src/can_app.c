/**
  ******************************************************************************
  * @file    can_app.c
  * @brief   FDCAN1 통신 골격 구현
  *
  *          - 수신: 전체 ID 허용 → RX FIFO0 → 인터럽트 콜백에서 링버퍼로
  *          - 송신: 클래식 CAN, TX FIFO
  *          - LED: 송신 = CAN_TX_LED, 수신 = CAN_RX_LED 30ms 펄스(액티브 로우),
  *                 버스오프 = CAN_ERR_LED 점등(복구되면 소등)
  *          - 버스오프: RM0444 절차대로 CCCR.INIT을 클리어해 자동 복구
  ******************************************************************************
  */
#include "can_app.h"
#include "main.h"
#include "fdcan.h"

#define CAN_RX_QLEN        16U    /* 수신 링버퍼 크기 */
#define CAN_LED_PULSE_MS   30U    /* 활동 LED 점등 시간 */

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

int CAN_App_Init(void)
{
    /* 골격 단계: 모든 표준 ID를 FIFO0로 수신 (프로토콜 확정 후 필터 축소) */
    FDCAN_FilterTypeDef f = { 0 };
    f.IdType       = FDCAN_STANDARD_ID;
    f.FilterIndex  = 0;
    f.FilterType   = FDCAN_FILTER_MASK;
    f.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    f.FilterID1    = 0x000;
    f.FilterID2    = 0x000;          /* 마스크 0 = 전체 통과 */
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &f) != HAL_OK)
    {
        return 0;
    }

    /* 필터에 안 걸린 프레임/리모트 프레임 처리 방침 */
    if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
                                     FDCAN_ACCEPT_IN_RX_FIFO0,  /* 표준 ID       */
                                     FDCAN_REJECT,              /* 확장 ID 거부  */
                                     FDCAN_REJECT_REMOTE,
                                     FDCAN_REJECT_REMOTE) != HAL_OK)
    {
        return 0;
    }

    /* 수신 + 버스오프 인터럽트 활성화 (NVIC의 TIM16_FDCAN_IT0로 들어옴) */
    if (HAL_FDCAN_ActivateNotification(&hfdcan1,
            FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_BUS_OFF, 0) != HAL_OK)
    {
        return 0;
    }

    return (HAL_FDCAN_Start(&hfdcan1) == HAL_OK) ? 1 : 0;
}

int CAN_App_Send(uint32_t std_id, const uint8_t *data, uint8_t len)
{
    if (len > 8U)
    {
        len = 8U;
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
        return 0;   /* TX FIFO 가득 참 (버스 문제 등) — 프레임 버림 */
    }

    /* 송신 활동 LED 펄스 시작 (액티브 로우 = RESET이 점등) */
    HAL_GPIO_WritePin(CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin, GPIO_PIN_RESET);
    tx_led_t0 = HAL_GetTick();
    return 1;
}

int CAN_App_SendKeyEvent(uint8_t key, uint8_t pressed, uint16_t mask)
{
    /* 임시 포맷 — SU-4100 프로토콜 확정 시 교체 */
    uint8_t d[4] = { key, pressed, (uint8_t)mask, (uint8_t)(mask >> 8) };
    return CAN_App_Send(CAN_TXID_KEY_EVENT, d, sizeof(d));
}

int CAN_App_PopRx(CanRxMsg *msg)
{
    if (rx_tail == rx_head)
    {
        return 0;
    }
    *msg = rx_q[rx_tail];
    rx_tail = (uint8_t)((rx_tail + 1U) % CAN_RX_QLEN);
    return 1;
}

void CAN_App_Process(void)
{
    uint32_t now = HAL_GetTick();

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

void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
    (void)hfdcan;
    if (ErrorStatusITs & FDCAN_IT_BUS_OFF)
    {
        bus_off = 1;
        HAL_GPIO_WritePin(CAN_ERR_LED_GPIO_Port, CAN_ERR_LED_Pin, GPIO_PIN_RESET);
    }
}
