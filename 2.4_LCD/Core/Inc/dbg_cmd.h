/**
  ******************************************************************************
  * @file    dbg_cmd.h
  * @brief   USB 콘솔 테스트 명령 (메인 보드 없이 화면·프로토콜 검증용)
  *
  *          Tera Term 등에서 한 줄씩 입력한다 (Enter로 확정, 입력 문자는 에코됨).
  *            help                                     명령 목록
  *            disp <unit> <sid_mm> <arm_deg> <det_deg> 0x02 표시 데이터 1회 주입 (unit 0=cm 1=inch)
  *            height <mm>                              0x03 ARM 높이 1회 주입
  *            state <0|1|off>                          0x04 상태를 1초마다 반복 주입 (0 READY, 1 EMERGENCY)
  *            unit <0|1>                               sim 시나리오가 쓰는 단위
  *            sim <on|key|off>                         on = 자동 시나리오 (SID→높이→ARM각→DET각 순서로
  *                                                     움직이고 1.5초 쉼, 300ms 주기로 0x02+0x03 주입)
  *                                                     key = 보드의 물리 키를 누르고 있는 동안 해당 값이
  *                                                     움직임 (메인 없이 키→화면 반응 확인용)
  *
  *          주입은 CAN 수신과 같은 해석 코드를 거친다(CAN_App_InjectRx).
  *          실제 메인이 붙어 있을 때 sim을 켜면 값이 뒤섞이므로 그때는 끈다.
  ******************************************************************************
  */
#ifndef DBG_CMD_H
#define DBG_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

/* main 루프(AppMain_Poll)에서 매회 호출: 콘솔 입력 파싱 + sim/state 주기 주입 */
void DbgCmd_Poll(void);

#ifdef __cplusplus
}
#endif

#endif /* DBG_CMD_H */
