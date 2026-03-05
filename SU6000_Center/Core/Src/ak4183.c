#include "ak4183.h"
#include "i2c.h"

/* The I2C instance connected to AK4183 */
extern I2C_HandleTypeDef hi2c2;

void AK4183_Init(void) {
  /* AK4183 Power-on Sequence (Datasheet p.11):
   * Send a command with PD0=0 to set Auto Power-Down mode with PENIRQ enabled.
   * Use X-axis command (0xC0) to put the chip in a known state.
   */
    uint8_t dummy_buf[2];

    /* Send X-axis command with PD0=0 via combined write-read (Repeated Start) */
    HAL_I2C_Mem_Read(&hi2c2, AK4183_I2C_ADDR, AK4183_CMD_READ_X,
                     I2C_MEMADD_SIZE_8BIT, dummy_buf, 2, 100);
}

/*
 * Reads X and Y ADC values from the AK4183
 * Includes a simple 5-sample average filter to reduce resistive panel noise
 * (jitter)
 */
uint8_t AK4183_ReadXY(uint16_t *x, uint16_t *y) {
  uint8_t buf[2];
  uint32_t sum_x = 0, sum_y = 0;
  const int num_samples = 3;

  for (int i = 0; i < num_samples; i++) {
    /* HAL_I2C_Mem_Read does: START->Addr(W)->Cmd->RepeatedStart->Addr(R)->Read->STOP
     * This keeps the driver switches ON during sampling (AK4183 requires Repeated Start) */
    if (HAL_I2C_Mem_Read(&hi2c2, AK4183_I2C_ADDR, AK4183_CMD_READ_X,
                         I2C_MEMADD_SIZE_8BIT, buf, 2, 50) != HAL_OK)
      return 0;
    sum_x += ((uint16_t)buf[0] << 4) | ((uint16_t)buf[1] >> 4);

    if (HAL_I2C_Mem_Read(&hi2c2, AK4183_I2C_ADDR, AK4183_CMD_READ_Y,
                         I2C_MEMADD_SIZE_8BIT, buf, 2, 50) != HAL_OK)
      return 0;
    sum_y += ((uint16_t)buf[0] << 4) | ((uint16_t)buf[1] >> 4);
  }

  *x = sum_x / num_samples;
  *y = sum_y / num_samples;
  return 1;
}
