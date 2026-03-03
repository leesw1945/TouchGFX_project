#include "ak4183.h"
#include "i2c.h"

/* The I2C instance connected to AK4183 */
extern I2C_HandleTypeDef hi2c2;

void AK4183_Init(void) {
  /* AK4183 does not generally require complex init over I2C before the first
     read. You can add power-mode specific commands here if needed according to
     your PDF. */
}

/*
 * Reads X and Y ADC values from the AK4183
 * Includes a simple 5-sample average filter to reduce resistive panel noise
 * (jitter)
 */
uint8_t AK4183_ReadXY(uint16_t *x, uint16_t *y) {
  uint8_t cmd_x = AK4183_CMD_READ_X;
  uint8_t cmd_y = AK4183_CMD_READ_Y;
  uint8_t buf[2];
  uint32_t sum_x = 0, sum_y = 0;
  const int num_samples = 5;

  for (int i = 0; i < num_samples; i++) {
    /* Request X measurement and read 2 bytes (12-bit ADC result) */
    if (HAL_I2C_Master_Transmit(&hi2c2, AK4183_I2C_ADDR, &cmd_x, 1, 5) !=
        HAL_OK)
      return 0;
    if (HAL_I2C_Master_Receive(&hi2c2, AK4183_I2C_ADDR, buf, 2, 5) != HAL_OK)
      return 0;
    sum_x += ((uint16_t)buf[0] << 4) | ((uint16_t)buf[1] >> 4);

    /* Request Y measurement and read 2 bytes (12-bit ADC result) */
    if (HAL_I2C_Master_Transmit(&hi2c2, AK4183_I2C_ADDR, &cmd_y, 1, 5) !=
        HAL_OK)
      return 0;
    if (HAL_I2C_Master_Receive(&hi2c2, AK4183_I2C_ADDR, buf, 2, 5) != HAL_OK)
      return 0;
    sum_y += ((uint16_t)buf[0] << 4) | ((uint16_t)buf[1] >> 4);
  }

  *x = sum_x / num_samples;
  *y = sum_y / num_samples;
  return 1;
}
