/**
  ******************************************************************************
  * @file    dbg_console.c
  * @brief   USB CDC 기반 디버그 콘솔 구현
  *
  *          syscalls.c의 weak _write()를 오버라이드해 printf를 CDC로 보낸다.
  *          CDC_Transmit_FS()는 호출자 버퍼를 복사 없이 그대로 USB 전송에
  *          사용하므로, 전송이 끝날 때까지 유효한 자체 복사 버퍼를 거친다.
  ******************************************************************************
  */
#include "dbg_console.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
#include <string.h>

extern USBD_HandleTypeDef hUsbDeviceFS;

#define DBG_TX_CHUNK        512U  /* 전송 복사 버퍼 크기 */
#define DBG_TX_TIMEOUT_MS   20U   /* 이전 전송 완료 대기 한도 (초과 시 로그 버림) */
#define DBG_RX_RING_SIZE    256U

static uint8_t tx_copy[DBG_TX_CHUNK];

static uint8_t           rx_ring[DBG_RX_RING_SIZE];
static volatile uint16_t rx_head; /* USB ISR에서만 갱신 */
static volatile uint16_t rx_tail; /* 메인 루프에서만 갱신 */

void dbg_console_init(void)
{
  /* printf 즉시 출력 (newlib 기본 버퍼링 제거) */
  setvbuf(stdout, NULL, _IONBF, 0);
}

static int usb_ready(void)
{
  return (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED);
}

/* syscalls.c의 weak _write 오버라이드 — printf가 여기로 온다 */
int _write(int file, char *ptr, int len)
{
  (void)file;

  if (len <= 0 || !usb_ready())
  {
    return len; /* 호스트 미연결: 버림 (블로킹 금지) */
  }

  int sent = 0;
  while (sent < len)
  {
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;
    if (hcdc == NULL)
    {
      return len;
    }

    /* 이전 전송이 tx_copy를 아직 쓰고 있으면 완료까지 대기 (타임아웃 시 버림) */
    uint32_t t0 = HAL_GetTick();
    while (hcdc->TxState != 0U)
    {
      if (!usb_ready() || (HAL_GetTick() - t0) > DBG_TX_TIMEOUT_MS)
      {
        return len;
      }
    }

    uint16_t chunk = (uint16_t)(((uint32_t)(len - sent) > DBG_TX_CHUNK) ? DBG_TX_CHUNK
                                                                        : (uint32_t)(len - sent));
    memcpy(tx_copy, ptr + sent, chunk);

    if (CDC_Transmit_FS(tx_copy, chunk) != USBD_OK)
    {
      return len;
    }
    sent += chunk;
  }
  return len;
}

/* USB 수신 ISR 컨텍스트에서 호출됨 — 가득 차면 나머지는 버림 */
void dbg_console_rx_from_usb(const uint8_t *buf, uint32_t len)
{
  for (uint32_t i = 0; i < len; i++)
  {
    uint16_t next = (uint16_t)((rx_head + 1U) % DBG_RX_RING_SIZE);
    if (next == rx_tail)
    {
      break;
    }
    rx_ring[rx_head] = buf[i];
    rx_head = next;
  }
}

int dbg_getchar(void)
{
  if (rx_tail == rx_head)
  {
    return -1;
  }
  uint8_t c = rx_ring[rx_tail];
  rx_tail = (uint16_t)((rx_tail + 1U) % DBG_RX_RING_SIZE);
  return c;
}

uint32_t dbg_read(uint8_t *buf, uint32_t maxlen)
{
  uint32_t n = 0;
  while (n < maxlen)
  {
    int c = dbg_getchar();
    if (c < 0)
    {
      break;
    }
    buf[n++] = (uint8_t)c;
  }
  return n;
}
