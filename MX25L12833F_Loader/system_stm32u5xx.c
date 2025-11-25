/**
  ******************************************************************************
  * @file    system_stm32u5xx.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M33 Device System Source File for STM32U5xx devices.
  *          Minimal version for External Loader
  ******************************************************************************
  */

#include "stm32u5xx.h"

/* System Clock Frequency (Core Clock) */
uint32_t SystemCoreClock = 4000000U;  /* Initial value of MSI */

/* System Clock Frequency Table */
const uint8_t AHBPrescTable[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 
                                   1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};
const uint8_t APBPrescTable[8] = {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};
const uint32_t MSIRangeTable[16] = {48000000U, 24000000U, 16000000U, 12000000U,
                                     4000000U,  2000000U,  1330000U,  1000000U,
                                     3072000U,  1536000U,  1024000U,  768000U,
                                     400000U,   200000U,   133000U,   100000U};

/**
  * @brief  Setup the microcontroller system.
  * @retval None
  */
void SystemInit(void)
{
  /* FPU settings - set CP10 and CP11 Full Access */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
  SCB->CPACR |= ((3UL << 20U) | (3UL << 22U));
#endif

  /* Reset the RCC clock configuration to the default reset state */
  /* Set MSION bit */
  RCC->CR = RCC_CR_MSISON;

  /* Reset CFGR register */
  RCC->CFGR1 = 0U;
  RCC->CFGR2 = 0U;
  RCC->CFGR3 = 0U;

  /* Reset HSEON, CSSON, HSION, PLLxON bits */
  RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_HSIKERON | RCC_CR_HSION | 
               RCC_CR_PLL1ON | RCC_CR_PLL2ON | RCC_CR_PLL3ON);

  /* Reset PLLCFGR register */
  RCC->PLL1CFGR = 0U;
  RCC->PLL2CFGR = 0U;
  RCC->PLL3CFGR = 0U;

  /* Reset HSEBYP bit */
  RCC->CR &= ~RCC_CR_HSEBYP;

  /* Disable all interrupts */
  RCC->CIER = 0U;

  /* Configure the Vector Table location */
  SCB->VTOR = 0x20000000U;  /* RAM execution for External Loader */
}

/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
  uint32_t tmp, msirange, pllr, pllsource, pllm;

  /* Get MSI Range frequency */
  if(RCC->ICSCR1 & RCC_ICSCR1_MSIRGSEL)
  {
    /* MSIRGSEL = 1, use RCC_CR MSIRANGE */
    msirange = (RCC->CR & RCC_CR_MSIRANGE) >> RCC_CR_MSIRANGE_Pos;
  }
  else
  {
    /* MSIRGSEL = 0, use RCC_CSR MSISRANGE */
    msirange = (RCC->CSR & RCC_CSR_MSISRANGE) >> RCC_CSR_MSISRANGE_Pos;
  }

  /* Get Core Clock Frequency */
  tmp = (RCC->CFGR1 & RCC_CFGR1_SWS) >> RCC_CFGR1_SWS_Pos;
  
  switch (tmp)
  {
    case 0x00:  /* MSI used as system clock source */
      SystemCoreClock = MSIRangeTable[msirange];
      break;
      
    case 0x01:  /* HSI used as system clock source */
      SystemCoreClock = 16000000U;
      break;
      
    case 0x02:  /* HSE used as system clock source */
      SystemCoreClock = HSE_VALUE;
      break;
      
    case 0x03:  /* PLL used as system clock source */
      /* PLL1 configuration */
      pllsource = (RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1SRC);
      pllm = ((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1M) >> RCC_PLL1CFGR_PLL1M_Pos) + 1U;
      
      switch (pllsource)
      {
        case 0x00:  /* No clock source */
          SystemCoreClock = 0U;
          break;
          
        case 0x01:  /* MSI used as PLL clock source */
          SystemCoreClock = MSIRangeTable[msirange] / pllm;
          break;
          
        case 0x02:  /* HSI used as PLL clock source */
          SystemCoreClock = 16000000U / pllm;
          break;
          
        case 0x03:  /* HSE used as PLL clock source */
          SystemCoreClock = HSE_VALUE / pllm;
          break;
          
        default:
          SystemCoreClock = MSIRangeTable[msirange] / pllm;
          break;
      }
      
      /* PLL multiplication factor */
      SystemCoreClock *= ((RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1N) >> 
                         RCC_PLL1DIVR_PLL1N_Pos) + 1U;
      
      /* PLL division factor */
      pllr = ((RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1R) >> 
              RCC_PLL1DIVR_PLL1R_Pos) + 1U;
      SystemCoreClock /= pllr;
      break;
      
    default:
      SystemCoreClock = MSIRangeTable[msirange];
      break;
  }
  
  /* Compute AHB frequency */
  tmp = AHBPrescTable[((RCC->CFGR2 & RCC_CFGR2_HPRE) >> RCC_CFGR2_HPRE_Pos)];
  SystemCoreClock >>= tmp;
}
