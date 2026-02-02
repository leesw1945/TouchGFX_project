/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : External Loader 디버깅용 Main 프로그램
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * ★★★ External Loader 디버깅 설정 ★★★
 * loader_src.c의 DEBUG_MODE가 활성화되어 있어야 정상 디버깅 가능
 *
 * 확인 사항:
 * 1. loader_src.c에서 #define DEBUG_MODE 활성화 확인
 * 2. 또는 프로젝트 설정에서 DEBUG_MODE 매크로 추가
 * 3. 최적화 레벨: None (-O0)
 * 4. Debug 레벨: Maximum (-g3)
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
// 디버깅용 전역 변수
volatile uint32_t g_test_step = 0;
volatile int g_test_result = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* External Loader 함수들 */
extern int Init(void);
extern int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress);
extern int Write(uint32_t Address, uint32_t Size, uint8_t *buffer);
extern int Read(uint32_t Address, uint32_t Size, uint8_t *buffer);

void Test_ExternalLoader(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  External Loader 기능 테스트
 * @note   각 단계별로 브레이크포인트를 설정하여 디버깅
 */
void Test_ExternalLoader(void) {
  int result;

  uint8_t write_data[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  uint8_t read_data[16] = {0};

  uint32_t test_addr = 0x000000; // 상대 주소값

  /* ========================================================================
   * 테스트 시작
   * ======================================================================== */
  g_test_step = 0;

  /* ========================================================================
   * Step 1: Init Test
   * Flash 메모리 초기화
   * ======================================================================== */
  g_test_step = 1;
  result = Init();
  g_test_result = result;

  if (result != 1) {
    // 초기화 실패 - 여기에 브레이크포인트 설정
    g_test_step = 0xFF;
    while (1) {
      __NOP();
    }
  }

  /* ========================================================================
   * Step 2: Erase Test
   * 4KB Sector 0 지우기 (0x000000 ~ 0x000FFF)
   * ======================================================================== */
  g_test_step = 2;
  result = SectorErase(test_addr, test_addr + 0xFFF);
  g_test_result = result;

  if (result != 1) {
    // 지우기 실패 - 여기에 브레이크포인트 설정
    g_test_step = 0xFE;
    while (1) {
      __NOP();
    }
  }

  /* ========================================================================
   * Step 3: Erase Verification
   * 지워진 영역은 모두 0xFF 여야 함
   * ======================================================================== */
  g_test_step = 3;
  result = Read(test_addr, 16, read_data);

  for (int i = 0; i < 16; i++) {
    if (read_data[i] != 0xFF) {
      // 지우기 검증 실패 - 여기에 브레이크포인트 설정
      g_test_step = 0xFD;
      while (1) {
        __NOP();
      }
    }
  }

  /* ========================================================================
   * Step 4: Write Test
   * 테스트 데이터 16바이트 쓰기
   * ======================================================================== */
  g_test_step = 4;
  result = Write(test_addr, 16, write_data);
  g_test_result = result;

  if (result != 1) {
    // 쓰기 실패 - 여기에 브레이크포인트 설정
    g_test_step = 0xFC;
    while (1) {
      __NOP();
    }
  }

  /* ========================================================================
   * Step 5: Read Verification
   * 쓴 데이터와 읽은 데이터가 일치해야 함
   * ======================================================================== */
  g_test_step = 5;
  memset(read_data, 0, 16);
  result = Read(test_addr, 16, read_data);

  if (memcmp(write_data, read_data, 16) != 0) {
    // 읽기/검증 실패 - 여기에 브레이크포인트 설정
    g_test_step = 0xFB;
    while (1) {
      __NOP();
    }
  }

  /* ========================================================================
   * 모든 테스트 성공!
   * ======================================================================== */
  g_test_step = 0xAA;  // 성공 표시
  __NOP();  // 여기에 브레이크포인트 설정 - 성공 확인
}

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();  // ← 여기에 브레이크포인트 설정 - HAL 초기화 확인

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();  // ← 여기에 브레이크포인트 설정 - 클럭 설정 확인

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */

  // External Loader 테스트 시작
  // main()에 정상 진입했는지 확인하기 위해 여기에 브레이크포인트 설정
  Test_ExternalLoader();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    g_test_step++;  // 무한 루프 도달 확인용
    HAL_Delay(1000);
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 1;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
  PeriphClkInit.OspiClockSelection = RCC_OSPICLKSOURCE_PLL1;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
    __NOP();
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
