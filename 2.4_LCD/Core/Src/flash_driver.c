/**
  ******************************************************************************
  * @file    flash_driver.c
  * @brief   MX25L6433F SPI NOR 플래시 읽기 드라이버 + TouchGFX DataReader 훅
  *
  *          사용자 파일 — CubeMX / TouchGFX Designer 재생성 시 덮어쓰지 않음.
  *
  *          배선 (BUCKY_KEY_DISPLAY_REV01):
  *            SPI2 (PB13 SCK / PB14 MISO / PB15 MOSI), 32 Mbit/s
  *            SPI2_NCS (PB12) : 플래시 칩셀렉트, 액티브 로우
  *            TX DMA = DMA1_Channel3, RX DMA = DMA1_Channel2
  *
  *          TouchGFX는 이 플래시에서 이미지/폰트 데이터를 스트리밍하면서
  *          UI를 렌더링한다: TouchGFXGeneratedDataReader가 아래 3개의
  *          DataReader_* 훅을 통해 FAST READ(0x0B)를 요청한다. 라인 버퍼
  *          2개를 써서 N+1번째 라인의 DMA 읽기가 N번째 라인의 렌더링과
  *          겹쳐서 진행된다.
  ******************************************************************************
  */
#include "flash_driver.h"
#include "main.h"
#include "spi.h"

/* 여기서 사용하는 MX25L6433F 커맨드 (읽기 전용 — 굽기는 STM32CubeProgrammer
 * 외부 로더가 담당한다. FlashLoader/ 참고) */
#define FLASH_CMD_RDID       0x9FU   /* JEDEC ID 읽기                        */
#define FLASH_CMD_FAST_READ  0x0BU   /* Fast Read: 커맨드+주소24+더미 1바이트 */

#define FLASH_ADDR_MASK      0x00FFFFFFUL

static volatile int rx_busy = 0;     /* DMA 읽기가 진행 중                   */

static inline void FLASH_CS_Low(void)
{
    HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_RESET);
}

static inline void FLASH_CS_High(void)
{
    HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_SET);
}

/* FAST READ 헤더 전송: 커맨드, 24비트 주소(MSB 먼저), 더미 1바이트.
 * 이후에도 CS는 Low 유지 — 곧바로 데이터 구간이 이어진다. */
static void FLASH_SendReadHeader(uint32_t addr)
{
    uint8_t hdr[5] = {
        FLASH_CMD_FAST_READ,
        (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr,
        0x00
    };
    FLASH_CS_Low();
    HAL_SPI_Transmit(&hspi2, hdr, sizeof(hdr), HAL_MAX_DELAY);
}

uint32_t FLASH_DRIVER_ReadJEDEC(void)
{
    uint8_t cmd = FLASH_CMD_RDID;
    uint8_t id[3] = { 0 };

    FLASH_CS_Low();
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi2, id, sizeof(id), HAL_MAX_DELAY);
    FLASH_CS_High();

    return ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
}

int FLASH_DRIVER_Init(void)
{
    FLASH_CS_High();
    return (FLASH_DRIVER_ReadJEDEC() == FLASH_JEDEC_ID_MX25L6433F) ? 1 : 0;
}

/* TouchGFX DataReader 훅 -------------------------------------------------------*/

/* 작은 전송: CPU 블로킹 읽기 (DMA 설정 오버헤드보다 빠르다) */
void DataReader_ReadData(uint32_t address24, uint8_t *buffer, uint32_t length)
{
    FLASH_SendReadHeader(address24 & FLASH_ADDR_MASK);
    HAL_SPI_Receive(&hspi2, buffer, (uint16_t)length, HAL_MAX_DELAY);
    FLASH_CS_High();
}

/* 큰 전송: RX DMA 읽기를 시작해 두고 즉시 리턴 — 데이터가 도착하는 동안
 * TouchGFX는 이전 라인의 렌더링을 계속한다. */
void DataReader_StartDMAReadData(uint32_t address24, uint8_t *buffer, uint32_t length)
{
    rx_busy = 1;
    FLASH_SendReadHeader(address24 & FLASH_ADDR_MASK);
    HAL_SPI_Receive_DMA(&hspi2, buffer, (uint16_t)length);
}

void DataReader_WaitForReceiveDone(void)
{
    while (rx_busy)
    {
    }
}

/* SPI RX DMA 완료. SPI2 = 플래시 읽기 끝.
 * (전이중 마스터의 수신 DMA 완료는 HAL이 이 콜백으로 전달한다.) */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2)
    {
        FLASH_CS_High();
        rx_busy = 0;
    }
}
