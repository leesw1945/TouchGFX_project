/**
  ******************************************************************************
  * @file    dbg_console.h
  * @brief   USB CDC 기반 디버그 콘솔 (printf 리다이렉트 + 수신 링버퍼)
  *
  *          - printf()가 USB CDC 가상 COM 포트로 출력된다.
  *          - 호스트(PC)가 연결되지 않았으면 출력은 조용히 버려진다(블로킹 없음).
  *          - 수신 데이터는 링버퍼에 쌓이며 dbg_getchar()/dbg_read()로 읽는다.
  *          - printf는 메인 루프 컨텍스트에서만 호출할 것 (ISR에서 호출 금지).
  ******************************************************************************
  */
#ifndef DBG_CONSOLE_H
#define DBG_CONSOLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* main()의 USER CODE 2에서 1회 호출 (stdout 버퍼링 해제) */
void dbg_console_init(void);

/* USB 수신 콜백(usbd_cdc_if.c의 CDC_Receive_FS)에서 호출 — 사용자 직접 호출 금지 */
void dbg_console_rx_from_usb(const uint8_t *buf, uint32_t len);

/* 수신 1바이트 읽기. 비어 있으면 -1 (논블로킹) */
int dbg_getchar(void);

/* 수신 버퍼에서 최대 maxlen 바이트 읽기. 읽은 바이트 수 반환 (논블로킹) */
uint32_t dbg_read(uint8_t *buf, uint32_t maxlen);

#ifdef __cplusplus
}
#endif

#endif /* DBG_CONSOLE_H */
