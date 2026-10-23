/**
  ******************************************************************************
  * @file    dbg_cmd.c
  * @brief   USB 콘솔 테스트 명령 구현 (dbg_cmd.h 참조)
  ******************************************************************************
  */
#include "dbg_cmd.h"
#include "dbg_console.h"
#include "can_app.h"
#include "key_scan.h"
#include "display_driver.h"   /* diag_te_count 등 */
#include "main.h"
#include <stdio.h>
#include <string.h>

#define LINE_MAX          48U
#define SIM_PERIOD_MS     300U    /* OP의 0x02/0x0A 송신 주기와 동일 (OP 담당 확인: 300ms) */
#define SIM_PAUSE_TICKS   5U      /* 단계 사이 정지 = 5 x 300ms = 1.5초 */

static char     line[LINE_MAX];
static uint8_t  line_len;

/* ---- sim 시나리오: 4개 값을 순서대로 하나씩 움직인다 ----
 * 인덱스 0 SID mm, 1 ARM 높이 mm, 2 ARM 각도, 3 DET 각도 */
static uint8_t  sim_on;
static uint8_t  sim_unit;                                    /* 0 cm, 1 inch */
static uint32_t sim_t0;
static uint8_t  sim_phase;
static uint8_t  sim_pause;
static uint8_t  sim_toward_max[4];
static int16_t  sim_val[4]        = { 1000,  800, -30, -45 };
static const int16_t sim_min[4]   = {  500,  400, -30, -45 };   /* SID는 100cm 미만도 가능, 높이 40~170cm */
static const int16_t sim_max[4]   = { 1800, 1700, 120,  45 };
static const int16_t sim_step[4]  = {   50,   50,  10,   6 };

/* ---- sim key: 보드의 물리 키로 값을 직접 움직인다 (메인 없이 키→화면 반응 확인용) ----
 * 300ms마다 누르고 있는 키에 따라 값을 한 칸씩 바꿔 0x02+0x0A로 주입한다.
 * 키 번호는 물리 cKEY 번호(LCD 마주본 기준 왼쪽 열 0~5, 오른쪽 열 6~11). EMERGENCY 중에는 무시. */
static uint8_t  sim_key;
#define KSIM_SID_STEP     10   /* mm / 300ms → 화면 1cm씩  */
#define KSIM_HEIGHT_STEP  10   /* mm / 300ms → 화면 1cm씩  */
#define KSIM_ANGLE_STEP   1    /* 도 / 300ms → 화면 1도씩  */

static int16_t clamp16(int32_t v, int16_t lo, int16_t hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : (int16_t)v;
}

static void sim_key_step(void)
{
    const uint16_t k = CAN_App_IsEmergency() ? 0U : KEY_GetStableMask();
    int32_t sid = sim_val[0], height = sim_val[1], arm = sim_val[2], det = sim_val[3];

    if (k & (1U << 0))  height += KSIM_HEIGHT_STEP;   /* KEY0  ARM_UP            */
    if (k & (1U << 6))  height -= KSIM_HEIGHT_STEP;   /* KEY6  ARM_DOWN          */
    if (k & (1U << 1))  arm    -= KSIM_ANGLE_STEP;    /* KEY1  ARM_LEFT_ROT      */
    if (k & (1U << 7))  arm    += KSIM_ANGLE_STEP;    /* KEY7  ARM_RIGHT_ROT     */
    if (k & ((1U << 2) | (1U << 3))) sid += KSIM_SID_STEP;   /* KEY2 TUBE_RIGHT_SLIDE, KEY3 DET_RIGHT_SLIDE */
    if (k & ((1U << 8) | (1U << 9))) sid -= KSIM_SID_STEP;   /* KEY8 TUBE_LEFT_SLIDE,  KEY9 DET_LEFT_SLIDE  */
    if (k & (1U << 4))  det    -= KSIM_ANGLE_STEP;    /* KEY4  DET_LEFT_ROT      */
    if (k & (1U << 10)) det    += KSIM_ANGLE_STEP;    /* KEY10 DET_RIGHT_ROT     */

    sim_val[0] = clamp16(sid,    sim_min[0], sim_max[0]);
    sim_val[1] = clamp16(height, sim_min[1], sim_max[1]);
    sim_val[2] = clamp16(arm,    sim_min[2], sim_max[2]);
    sim_val[3] = clamp16(det,    sim_min[3], sim_max[3]);
}

/* ---- diag: 1초마다 카운터 변화량을 재 둔다 (TE 인터럽트 속도 확인용) ---- */
static uint32_t diag_t0;
static uint32_t te_prev,   te_per_sec;
static uint32_t fake_prev, fake_per_sec;

static void print_diag(void)
{
    printf("TE/s=%lu fake/s=%lu fallback=%u blocks=%lu txabort=%lu SYSCLK=%luHz\r\n",
           (unsigned long)te_per_sec, (unsigned long)fake_per_sec,
           (unsigned)LCD_IsVsyncFallbackActive(), (unsigned long)diag_blocks_sent,
           (unsigned long)CAN_App_GetTxAbortCount(), (unsigned long)SystemCoreClock);
}

/* ---- state 명령값: -1 꺼짐(0x0A 반복 안 함), 0 READY, 1 EMERGENCY ----
 * 0 또는 1이면 실제 OP처럼 0x0A를 300ms마다 반복 주입한다 (sim이 켜져 있으면 sim이 대신 보낸다) */
static int8_t   state_hold = -1;
static uint32_t state_t0;

/* ==== 프레임 주입 ============================================================*/

static void inject_display(uint8_t unit, uint16_t sid_mm, int16_t arm_deg, int16_t det_deg)
{
    uint8_t d[8];
    d[0] = CMD_BUCK_DISPLAY;
    d[1] = unit;
    d[2] = (uint8_t)(sid_mm >> 8);
    d[3] = (uint8_t)sid_mm;
    d[4] = (uint8_t)((uint16_t)arm_deg >> 8);
    d[5] = (uint8_t)arm_deg;
    d[6] = (uint8_t)((uint16_t)det_deg >> 8);
    d[7] = (uint8_t)det_deg;
    CAN_App_InjectRx(d, 8U);
}

/* 0x0A 주입: OP 보드 구조체(공용체)로 프레임을 만들어 CAN 수신과 같은 해석 경로를 탄다.
 * emergency 인수는 사람 기준(1 = 비상)이고 와이어 값은 SDK 정의대로 EMG_ON(0)/EMG_OFF(1)로 넣는다. */
static void inject_payload(uint8_t unit, uint8_t emergency, uint16_t height_mm)
{
    OP_To_Bucky_Key_Union_t tx;
    uint8_t d[8] = { 0 };

    tx.Payload.Unit          = unit ? UNIT_Inch : UNIT_Cm;
    tx.Payload.Emergency     = emergency ? EMG_ON : EMG_OFF;
    tx.Payload.Swivel_Height = height_mm;        /* Cortex-M0+는 리틀엔디안 → OP 배치와 동일 */

    d[0] = CMD_BUCK_KEY_PAYLOAD;
    memcpy(&d[1], tx.Bytes, sizeof(tx.Bytes));   /* [1] Unit [2] Emergency [3] LSB [4] MSB */
    CAN_App_InjectRx(d, CMD_BUCK_KEY_PAYLOAD_DLC);
}

/* 현재 콘솔 상태(unit 명령의 단위, state 명령값, 높이값 sim_val[1])로 0x0A를 한 번 주입 */
static void inject_payload_now(void)
{
    inject_payload(sim_unit, (state_hold > 0) ? 1U : 0U, (uint16_t)sim_val[1]);
}

/* ==== sim 한 스텝 (300ms마다) =================================================*/

static void sim_step_once(void)
{
    if (sim_pause)
    {
        if (--sim_pause == 0U)
        {
            sim_phase = (uint8_t)((sim_phase + 1U) & 3U);
        }
        return;
    }

    const int16_t target = sim_toward_max[sim_phase] ? sim_max[sim_phase] : sim_min[sim_phase];
    const int16_t step   = sim_step[sim_phase];
    int16_t v = sim_val[sim_phase];

    if (v < target)
    {
        v = (int16_t)(v + step);
        if (v > target) v = target;
    }
    else if (v > target)
    {
        v = (int16_t)(v - step);
        if (v < target) v = target;
    }
    sim_val[sim_phase] = v;

    if (v == target)
    {
        sim_toward_max[sim_phase] ^= 1U;   /* 다음번엔 반대 방향 */
        sim_pause = SIM_PAUSE_TICKS;
    }
}

/* ==== 명령 파싱 ==============================================================*/

static void skip_spaces(const char **p)
{
    while (**p == ' ') (*p)++;
}

/* 부호 있는 10진 정수 하나 읽기. 성공 1 / 실패 0 */
static int next_int(const char **p, int *out)
{
    int sign = 1, v = 0, n = 0;
    skip_spaces(p);
    if (**p == '-') { sign = -1; (*p)++; }
    else if (**p == '+') { (*p)++; }
    while (**p >= '0' && **p <= '9')
    {
        v = v * 10 + (**p - '0');
        (*p)++;
        n++;
    }
    if (n == 0) return 0;
    *out = sign * v;
    return 1;
}

static void print_help(void)
{
    printf("명령 (Enter로 확정):\r\n"
           "  disp <unit> <sid_mm> <arm_deg> <det_deg>  0x02 주입 (unit 0=cm 1=inch)\r\n"
           "  height <mm>                              0x0A 1회 주입 (단위=unit, 비상=state 값)\r\n"
           "  state <0|1|off>                          0x0A 300ms 반복 주입 (0 READY 1 EMERGENCY, off=중지)\r\n"
           "  unit <0|1>                               sim 단위\r\n"
           "  diag                                     TE/s, 폴백, 송신취소 등 진단값\r\n"
           "  sim <on|key|off>                         on=자동 시나리오, key=보드 키로 값 이동 (300ms, 0x02+0x0A)\r\n"
           "  예) disp 0 1800 140 30 / height 1200 / state 1 / sim key\r\n");
}

static void run_line(const char *s)
{
    char cmd[8];
    uint8_t n = 0;
    int a, b, c, d;

    skip_spaces(&s);
    while (*s && *s != ' ' && n < sizeof(cmd) - 1U)
    {
        cmd[n++] = *s++;
    }
    cmd[n] = 0;
    if (n == 0U) return;

    if (strcmp(cmd, "help") == 0)
    {
        print_help();
    }
    else if (strcmp(cmd, "diag") == 0)
    {
        print_diag();
    }
    else if (strcmp(cmd, "disp") == 0)
    {
        if (next_int(&s, &a) && next_int(&s, &b) && next_int(&s, &c) && next_int(&s, &d))
        {
            inject_display((uint8_t)a, (uint16_t)b, (int16_t)c, (int16_t)d);
            printf("ok disp unit=%d sid=%dmm arm=%d det=%d\r\n", a, b, c, d);
        }
        else
        {
            printf("? disp <unit> <sid_mm> <arm_deg> <det_deg>\r\n");
        }
    }
    else if (strcmp(cmd, "height") == 0)
    {
        if (next_int(&s, &a))
        {
            sim_val[1] = (int16_t)a;             /* sim key의 시작값도 함께 맞춘다 */
            inject_payload_now();
            printf("ok height %dmm (0x0A 1회, %s)\r\n", a, (state_hold > 0) ? "EMERGENCY" : "READY");
        }
        else
        {
            printf("? height <mm>\r\n");
        }
    }
    else if (strcmp(cmd, "state") == 0)
    {
        skip_spaces(&s);
        if (strncmp(s, "off", 3) == 0)
        {
            state_hold = -1;
            printf("ok state off (0x0A 반복 중지 -> 1.5초 뒤 READY, 높이 대시)\r\n");
        }
        else if (next_int(&s, &a) && (a == 0 || a == 1))
        {
            state_hold = (int8_t)a;
            state_t0   = HAL_GetTick();
            inject_payload_now();
            printf("ok state %s (0x0A 300ms 반복, Emergency 바이트=%u)\r\n",
                   a ? "EMERGENCY" : "READY", a ? (unsigned)EMG_ON : (unsigned)EMG_OFF);
        }
        else
        {
            printf("? state <0|1|off>\r\n");
        }
    }
    else if (strcmp(cmd, "unit") == 0)
    {
        if (next_int(&s, &a) && (a == 0 || a == 1))
        {
            sim_unit = (uint8_t)a;
            printf("ok unit %s\r\n", a ? "inch" : "cm");
        }
        else
        {
            printf("? unit <0|1>\r\n");
        }
    }
    else if (strcmp(cmd, "sim") == 0)
    {
        skip_spaces(&s);
        if (strncmp(s, "on", 2) == 0)
        {
            sim_on    = 1;
            sim_key   = 0;
            sim_phase = 0;
            sim_pause = 0;
            sim_t0    = HAL_GetTick();
            for (uint8_t i = 0; i < 4U; i++) sim_toward_max[i] = 1U;
            printf("ok sim on (SID -> height -> ARM deg -> DET deg 순서, 300ms)\r\n");
        }
        else if (strncmp(s, "key", 3) == 0)
        {
            sim_key = 1;
            sim_on  = 0;
            sim_t0  = HAL_GetTick();
            printf("ok sim key (ARM UP/DOWN=높이, ARM ROT=ARM각, 슬라이드=SID, DET ROT=DET각)\r\n");
        }
        else if (strncmp(s, "off", 3) == 0)
        {
            sim_on  = 0;
            sim_key = 0;
            printf("ok sim off (1.5초 뒤 화면 대시)\r\n");
        }
        else
        {
            printf("? sim <on|key|off>\r\n");
        }
    }
    else
    {
        printf("? 알 수 없는 명령: %s (help)\r\n", cmd);
    }
}

/* ==== 주기 처리 ==============================================================*/

void DbgCmd_Poll(void)
{
    int c;

    /* 입력 문자 수집 (에코 포함) */
    while ((c = dbg_getchar()) >= 0)
    {
        if (c == '\r' || c == '\n')
        {
            printf("\r\n");
            line[line_len] = 0;
            if (line_len > 0U)
            {
                run_line(line);
            }
            line_len = 0;
        }
        else if (c == 0x08 || c == 0x7F)          /* 백스페이스 */
        {
            if (line_len > 0U)
            {
                line_len--;
                printf("\b \b");
            }
        }
        else if (c >= 0x20 && c < 0x7F && line_len < LINE_MAX - 1U)
        {
            line[line_len++] = (char)c;
            putchar(c);
        }
    }

    const uint32_t now = HAL_GetTick();

    /* 1초마다 TE/가짜 VSYNC 카운터의 초당 증가량 갱신 */
    if ((now - diag_t0) >= 1000U)
    {
        diag_t0      = now;
        te_per_sec   = diag_te_count - te_prev;
        te_prev      = diag_te_count;
        fake_per_sec = diag_fake_vsync - fake_prev;
        fake_prev    = diag_fake_vsync;
    }

    if ((sim_on || sim_key) && (now - sim_t0) >= SIM_PERIOD_MS)
    {
        sim_t0 = now;
        if (sim_on)
        {
            sim_step_once();
        }
        else
        {
            sim_key_step();
        }
        inject_display(sim_unit, (uint16_t)sim_val[0], sim_val[2], sim_val[3]);
        inject_payload_now();                     /* 0x0A: 단위 + state 값 + 높이 */
    }

    /* state 0/1이면 실제 OP처럼 0x0A를 주기적으로 반복 (sim 중에는 위에서 이미 보냈으므로 생략) */
    if (state_hold >= 0 && !sim_on && !sim_key && (now - state_t0) >= SIM_PERIOD_MS)
    {
        state_t0 = now;
        inject_payload_now();
    }
}
