/**
  ******************************************************************************
  * @file    boot_option.h
  * @brief   옵션 바이트 자동 설정 — USB DFU 진입을 위한 nBOOT_SEL 끄기
  *
  *          STM32G0의 옵션 바이트 nBOOT_SEL은 공장 기본값이 1이다. 1이면 BOOT0 핀을
  *          무시하고 항상 내부 플래시로 부팅하므로, JP1(BOOT0)을 쇼트해도 ROM
  *          부트로더(USB DFU)에 들어가지 못한다.
  *
  *          이 모듈은 부팅 때 nBOOT_SEL을 읽어 1이면 0으로 바꾸고 옵션 바이트를
  *          다시 로드한다(= 즉시 시스템 리셋). 보드당 한 번만 일어나며, 이후
  *          부팅에서는 이미 0이라 아무것도 하지 않는다. 양산에서 CubeProgrammer로
  *          옵션 바이트를 따로 끄는 공정이 필요 없어진다.
  *
  *          전제: 펌웨어가 이미 보드에 들어 있어야 한다(첫 굽기는 ST-LINK).
  *          주의: 이 코드가 들어간 펌웨어는 항상 nBOOT_SEL을 0으로 만든다.
  *                RDP를 1→0으로 내려 옵션이 초기화돼도 다음 부팅에서 다시 0이 된다.
  ******************************************************************************
  */
#ifndef BOOT_OPTION_H
#define BOOT_OPTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* main()의 USER CODE SysInit(클럭 설정 직후, 주변장치 초기화 전)에서 1회 호출.
 * nBOOT_SEL이 1이면 0으로 프로그램하고 리셋한다(돌아오지 않음).
 * 이미 0이면 즉시 리턴. 프로그램 실패 시에도 리턴한다(보드는 평소처럼 부팅). */
void BootOption_EnsureBoot0Pin(void);

/* 현재 nBOOT_SEL 값 (1 = BOOT0 핀 무시, 0 = BOOT0 핀 사용 = DFU 진입 가능) */
uint8_t BootOption_GetNBootSel(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_OPTION_H */
