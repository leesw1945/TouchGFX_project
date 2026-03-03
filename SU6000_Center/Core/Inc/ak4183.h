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

/* AK4183 Commands */
#define AK4183_CMD_READ_X 0xC0 /* X Position Measure */
#define AK4183_CMD_READ_Y 0xD0 /* Y Position Measure */

void AK4183_Init(void);
uint8_t AK4183_ReadXY(uint16_t *x, uint16_t *y);

#ifdef __cplusplus
}
#endif

#endif /* AK4183_H */
