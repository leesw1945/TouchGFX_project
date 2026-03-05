#ifndef AK4183_H
#define AK4183_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/*
 * AK4183 I2C Device Address (from Datasheet)
 * Base 6-bit address: "100100"
 * 7th bit: CAD0 pin (0 or 1)
 *
 * If CAD0 = 0: 7-bit = 0x48 (1001000) -> 8-bit HAL format = 0x90 (0x48 << 1)
 * If CAD0 = 1: 7-bit = 0x49 (1001001) -> 8-bit HAL format = 0x92 (0x49 << 1)
 */
#define AK4183_I2C_ADDR (0x49 << 1)

/* AK4183 Commands (from Datasheet)
 * Bit 7: S (Start) = 1
 * Bit 6-4: A2-A0 (Channel Select)
 * Bit 3: MODE (12-bit/8-bit) = 0 (12-bit)
 * Bit 2: SER/DFR (Single-Ended/Differential) = 0 (Differential)
 * Bit 1-0: PD1-PD0 (Power-Down Mode) = 00 (Power-Down between conversions.
 * PENIRQ enabled)
 *
 * X-Position (Measure Y-panel): Channel = 001/101 (often 0xD0 or 0xD0)
 * Y-Position (Measure X-panel): Channel = 101/001 (often 0x90 or 0xC0)
 */
/* Control Command Byte: S(1) | A2 | A1 | A0 | X1(0) | PD0(0) | MODE(0=12bit) | X2(0)
 * X-axis: S=1, A2=1, A1=0, A0=0 -> 0xC0
 * Y-axis: S=1, A2=1, A1=0, A0=1 -> 0xD0
 */
#define AK4183_CMD_READ_X 0xC0 /* X Position Measure: S=1,A2=1,A1=0,A0=0,PD0=0,MODE=0 */
#define AK4183_CMD_READ_Y 0xD0 /* Y Position Measure: S=1,A2=1,A1=0,A0=1,PD0=0,MODE=0 */

void AK4183_Init(void);
uint8_t AK4183_ReadXY(uint16_t *x, uint16_t *y);

#ifdef __cplusplus
}
#endif

#endif /* AK4183_H */
