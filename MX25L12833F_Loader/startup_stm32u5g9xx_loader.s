/**
  ******************************************************************************
  * @file    startup_stm32u5g9xx_loader.s
  * @author  MCD Application Team
  * @brief   Minimal startup file for STM32U5G9xx External Loader
  ******************************************************************************
  */

  .syntax unified
  .cpu cortex-m33
  .fpu fpv5-sp-d16
  .thumb

.global  g_pfnVectors

/* Start address for initialization values in RAM */
.word  _sidata
.word  _sdata
.word  _edata
.word  _sbss
.word  _ebss

  .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function

Reset_Handler:
  ldr   sp, =_estack     /* Set stack pointer */
  
/* Copy initialized data from flash to RAM */
  movs  r1, #0
  b  LoopCopyDataInit

CopyDataInit:
  ldr  r3, =_sidata
  ldr  r3, [r3, r1]
  str  r3, [r0, r1]
  adds  r1, r1, #4

LoopCopyDataInit:
  ldr  r0, =_sdata
  ldr  r3, =_edata
  adds  r2, r0, r1
  cmp  r2, r3
  bcc  CopyDataInit
  ldr  r2, =_sbss
  b  LoopFillZerobss

/* Zero fill the bss segment */
FillZerobss:
  movs  r3, #0
  str  r3, [r2], #4

LoopFillZerobss:
  ldr  r3, = _ebss
  cmp  r2, r3
  bcc  FillZerobss

/* Call Init function */
  bl  Init
  bx  lr

.size  Reset_Handler, .-Reset_Handler

/* Dummy exception handlers (infinite loops) */
  .section  .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
  .size  Default_Handler, .-Default_Handler

/* Vector table - minimal version for External Loader */
  .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object
  .size  g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  .word  _estack
  .word  Reset_Handler
  .word  0  /* NMI_Handler */
  .word  0  /* HardFault_Handler */
  .word  0  /* MemManage_Handler */
  .word  0  /* BusFault_Handler */
  .word  0  /* UsageFault_Handler */
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0  /* SVC_Handler */
  .word  0  /* DebugMon_Handler */
  .word  0
  .word  0  /* PendSV_Handler */
  .word  0  /* SysTick_Handler */
  
  /* External Interrupts - all set to 0 for External Loader */
  .rept 125
  .word  0
  .endr

/* Stack allocation */
.global  _estack
 .set  _estack, 0x20040000  /* End of RAM */
