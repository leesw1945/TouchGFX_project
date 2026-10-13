/**
  ******************************************************************************
  * @file    boot_option.c
  * @brief   옵션 바이트 자동 설정 구현 (boot_option.h 참조)
  *
  *          절차 (RM0444 옵션 바이트 프로그래밍):
  *            1) FLASH_OPTR의 nBOOT_SEL 비트 확인
  *            2) 플래시 CR 잠금 해제 → 옵션 바이트 잠금 해제
  *            3) HAL_FLASHEx_OBProgram: USER 옵션 중 nBOOT_SEL만 0으로
  *               (RDP 등 다른 옵션은 HAL이 현재 값을 유지한다)
  *            4) HAL_FLASH_OB_Launch: OBL_LAUNCH 세트 → 옵션 바이트 재로드 = 시스템 리셋
  ******************************************************************************
  */
#include "boot_option.h"
#include "main.h"

uint8_t BootOption_GetNBootSel(void)
{
    return (uint8_t)((FLASH->OPTR & FLASH_OPTR_nBOOT_SEL) ? 1U : 0U);
}

void BootOption_EnsureBoot0Pin(void)
{
    if (BootOption_GetNBootSel() == 0U)
    {
        return;                              /* 이미 BOOT0 핀 사용 — 할 일 없음 */
    }

    /* 옵션 바이트를 쓰는 동안 플래시가 멈추므로 인터럽트를 끈다.
     * 성공하면 어차피 리셋되고, 실패하면 아래에서 다시 켠다. */
    __disable_irq();

    if (HAL_FLASH_Unlock() == HAL_OK && HAL_FLASH_OB_Unlock() == HAL_OK)
    {
        FLASH_OBProgramInitTypeDef ob = { 0 };
        ob.OptionType = OPTIONBYTE_USER;     /* USER 옵션 바이트만 대상 */
        ob.USERType   = OB_USER_nBOOT_SEL;   /* 그중 nBOOT_SEL 비트만 */
        ob.USERConfig = OB_BOOT0_FROM_PIN;   /* = 0: BOOT0 신호를 PA14/BOOT0 핀에서 */

        if (HAL_FLASHEx_OBProgram(&ob) == HAL_OK)
        {
            (void)HAL_FLASH_OB_Launch();     /* 옵션 바이트 재로드 → 즉시 리셋, 돌아오지 않음 */
            while (1)
            {
                /* 리셋을 기다린다 (도달하지 않음) */
            }
        }

        (void)HAL_FLASH_OB_Lock();
        (void)HAL_FLASH_Lock();
    }

    __enable_irq();                          /* 실패: 보드는 평소처럼 부팅 (DFU 진입만 불가) */
}
