/*
 * touch_calibration.c
 *
 *  Created on: Mar 16, 2026
 *      Author: user
 */


#include "touch_calibration.h"
#include "stm32u5xx_hal.h"

TouchCalibData g_calibData;
uint8_t g_calibValid = 0;

static uint32_t CalcChecksum(const TouchCalibData* d) {
    return (uint32_t)(d->raw_x_min + d->raw_x_max + d->raw_y_min + d->raw_y_max) ^ d->magic;
}

uint8_t Calib_Load(TouchCalibData* data) {
    // Flash에서 직접 읽기 (메모리 매핑)
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

    // 1. Flash 잠금 해제
    HAL_FLASH_Unlock();

    // 2. 마지막 페이지 지우기
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks     = FLASH_BANK_2;          // 2MB 이후는 Bank 2
    erase.Page      = 255;                   // Bank 2의 마지막 페이지
    erase.NbPages   = 1;
    status = HAL_FLASHEx_Erase(&erase, &pageError);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return 0;
    }

    // 3. 데이터 쓰기 (STM32U5는 128비트=16바이트 단위)
    //    TouchCalibData를 16바이트 정렬해서 쓰기
    TouchCalibData writeData = *data;
    writeData.magic = CALIB_MAGIC;
    writeData.checksum = CalcChecksum(&writeData);

    uint32_t* src = (uint32_t*)&writeData;
    uint32_t  addr = CALIB_FLASH_ADDR;
    uint32_t  words = (sizeof(TouchCalibData) + 15) / 16 * 4;  // 128비트 단위

    for (uint32_t i = 0; i < words; i += 4) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, addr,
                                   (uint32_t)(src + i));
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return 0;
        }
        addr += 16;
    }

    // 4. Flash 잠금
    HAL_FLASH_Lock();
    return 1;
}
