/*
 * touch_calibration.c
 *
 *  Created on: Mar 16, 2026
 *      Author: user
 */


#include "touch_calibration.h"
#include "stm32u5xx_hal.h"
#include <stdio.h>

TouchCalibData g_calibData;
uint8_t g_calibValid = 0;

static uint32_t CalcChecksum(const TouchCalibData* d) {
    return (uint32_t)(d->raw_x_min + d->raw_x_max + d->raw_y_min + d->raw_y_max) ^ d->magic;
}

extern DCACHE_HandleTypeDef hdcache1;

uint8_t Calib_Load(TouchCalibData* data) {
    // Flash에서 직접 읽기 전에 해당 주소의 Cache 무효화 (항상 최신값 읽기 보장)
    HAL_DCACHE_InvalidateByAddr(&hdcache1, (const uint32_t*)CALIB_FLASH_ADDR, sizeof(TouchCalibData));
    
    TouchCalibData* flash = (TouchCalibData*)CALIB_FLASH_ADDR;

    if (flash->magic != CALIB_MAGIC) return 0;
    if (flash->checksum != CalcChecksum(flash)) return 0;

    *data = *flash;
    return 1;
}

uint8_t Calib_Save(const TouchCalibData* data) {
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase;
    uint32_t pageError = 0;

    // 1. Flash 잠금 해제 및 이전 에러 플래그 초기화
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_PROGERR | 
                           FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | 
                           FLASH_FLAG_PGSERR | FLASH_FLAG_OPTWERR);

    // 2. 마지막 페이지 지우기
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks     = FLASH_BANK_2;          // 2MB 이후는 Bank 2
    erase.Page      = 255;                   // Bank 2의 마지막 페이지
    erase.NbPages   = 1;
    status = HAL_FLASHEx_Erase(&erase, &pageError);
    if (status != HAL_OK) {
        printf("[Calib] FLASH Erase Failed! Status: %d, PageError: %lu\r\n", status, pageError);
        HAL_FLASH_Lock();
        return 0;
    }

    // 3. 데이터 쓰기 (STM32U5는 128비트=16바이트 배열 단위로 강제 정렬)
    // 스택 변수의 메모리 정렬(Alignment) 오류를 막기 위해 uint32_t 배열 사용
    uint32_t src[4] = {0};                 // 16바이트 버퍼
    TouchCalibData* temp = (TouchCalibData*)src;
    *temp = *data;                         // 기존 파라미터 복사
    temp->magic = CALIB_MAGIC;             // 매직넘버 주입
    temp->checksum = CalcChecksum(temp);   // 체크섬 계산 주입

    uint32_t addr = CALIB_FLASH_ADDR;

    // 128-bit (QuadWord) 쓰기 1방이면 16바이트가 전부 들어감.
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, addr, (uint32_t)src);
    if (status != HAL_OK) {
        printf("[Calib] FLASH Program Failed! Status: %d\r\n", status);
        HAL_FLASH_Lock();
        return 0;
    }

    printf("[Calib] Save Success! Magic: 0x%08lX, Csum: 0x%08lX\r\n", temp->magic, temp->checksum);

    // 4. 안전하게 DCACHE 동기화 및 Lock
    HAL_DCACHE_CleanInvalidateByAddr(&hdcache1, (const uint32_t*)addr, 16);
    HAL_FLASH_Lock();
    
    return 1;
}
