/**
  ******************************************************************************
  * @file    flash_driver.h
  * @brief   MX25L6433F SPI NOR 플래시 읽기 드라이버 + TouchGFX DataReader 훅
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  ******************************************************************************
  */
#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* MX25L6433F JEDEC ID: 제조사 0xC2, 타입 0x20, 용량 0x17 (64Mbit) */
#define FLASH_JEDEC_ID_MX25L6433F   0x00C22017UL

/* SPI2로 플래시 존재 확인 (JEDEC ID, 커맨드 0x9F).
 * MX25L6433F가 응답하면 1, 아니면 0 (배선/전원 문제). */
int      FLASH_DRIVER_Init(void);
uint32_t FLASH_DRIVER_ReadJEDEC(void);

/* TouchGFX DataReader 훅 — TouchGFXGeneratedDataReader.cpp가 호출한다.
 * `address24`에는 가상 주소(0x90xxxxxx)가 그대로 들어오므로, 드라이버가
 * 24비트 물리 플래시 주소로 마스킹해서 사용한다. */
void DataReader_WaitForReceiveDone(void);
void DataReader_ReadData(uint32_t address24, uint8_t *buffer, uint32_t length);
void DataReader_StartDMAReadData(uint32_t address24, uint8_t *buffer, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_DRIVER_H */
