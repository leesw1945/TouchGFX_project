/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "i2c.h"
#include "ltdc.h"
#include "gpio.h"
#include "fmc.h"
#include "app_touchgfx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "../../Drivers/CustomDriver/ak4183.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
//__attribute__((section(".TouchGFX_Framebuffer"))) __attribute__((aligned(32))) static uint32_t framebuffer[288000]; // RGB888

#define REFRESH_COUNT        1386
#define SDRAM_TIMEOUT                            ((uint32_t)0xFFFF)
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)


static FMC_SDRAM_CommandTypeDef Command;

extern AK4183_Handle_t ak4183_handle;  // STM32TouchController.cpp에서 정의됨

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram);

// 터치 테스트 함수 (디버그용)
static void Test_TouchController(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_FMC_Init();
  MX_LTDC_Init();
  MX_I2C2_Init();
  MX_TouchGFX_Init();
  /* USER CODE BEGIN 2 */
  SDRAM_Initialization_Sequence(&hsdram1);

  /* --- SDRAM 쓰기/읽기 테스트 변수 선언 --- */
//  uint32_t *sdram_address = (uint32_t*)0xD0000000; // Bank 2의 시작 주소로 수정
//  uint32_t write_data = 0xAAAAAAAA; // 테스트를 위해 쓸 값
//  uint32_t read_data = 0;
//  volatile uint32_t test_success = 0; // volatile 키워드 추가: 컴파일러 최적화 방지
//
//  /* --- 1. 쓰기 테스트 --- */
//  *sdram_address = write_data;
//
//  /* --- 2. 읽기 테스트 --- */
//  read_data = *sdram_address;
//
//  /* --- 3. 검증 --- */
//  if (read_data == write_data)
//  {
//    // 성공!
//    test_success = 1;
//  }
//  else
//  {
//    // 실패!
//    test_success = 0;
//  }

  // 터치 컨트롤러 테스트 (옵션)
  #ifdef TOUCH_DEBUG
  Test_TouchController();
  #endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

  MX_TouchGFX_Process();
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram)
{
  __IO uint32_t tmpmrd = 0;

  /* Step 1: Clock Configuration Enable */
  Command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
  Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
  Command.AutoRefreshNumber = 1;
  Command.ModeRegisterDefinition = 0;
  HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT);
  HAL_Delay(1); // 1ms

  /* Step 2: PALL (Precharge All) */
  Command.CommandMode = FMC_SDRAM_CMD_PALL;
  Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
  Command.AutoRefreshNumber = 1;
  Command.ModeRegisterDefinition = 0;
  HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT);

  /* Step 3: Auto-refresh */
  Command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
  Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
  Command.AutoRefreshNumber = 8;
  Command.ModeRegisterDefinition = 0;
  HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT);

  /* Step 4: Load Mode Register */
  tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2   |
                     SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |
                     SDRAM_MODEREG_CAS_LATENCY_3         |
                     SDRAM_MODEREG_OPERATING_MODE_STANDARD |
                     SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;
  Command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
  Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
  Command.AutoRefreshNumber = 1;
  Command.ModeRegisterDefinition = tmpmrd;
  HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT);

  /* Step 5: Set a clock configuration enable command */
  HAL_SDRAM_ProgramRefreshRate(hsdram, REFRESH_COUNT);
}

/**
  * @brief 터치 컨트롤러 테스트 함수
  * @note 디버그 목적으로 터치 동작을 확인
  */
static void Test_TouchController(void)
{
    AK4183_Handle_t test_handle;
    AK4183_TouchData_t touch_data;

    // 터치 컨트롤러 초기화
    if (AK4183_Init(&test_handle, &hi2c2) == AK4183_OK) {
        // LED 켜기 (초기화 성공)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);

        // 5초간 터치 테스트
        uint32_t start_time = HAL_GetTick();
        while ((HAL_GetTick() - start_time) < 5000) {
            // 터치 데이터 읽기
            if (AK4183_ReadTouch(&test_handle, &touch_data) == AK4183_OK) {
                if (touch_data.state == AK4183_TOUCH_PRESSED) {
                    // 터치됨 - LED 토글
                    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_1);

                    // 디버그 출력 (UART나 SWO 사용 시)
                    #ifdef USE_DEBUG_UART
                    printf("Touch: X=%d, Y=%d, Pressure=%d\n",
                           touch_data.x, touch_data.y, touch_data.pressure);
                    #endif
                }
            }
            HAL_Delay(50);  // 50ms 주기로 확인
        }
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
