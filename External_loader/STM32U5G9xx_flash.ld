/**
  ******************************************************************************
  * @file    STM32U5G9xx_flash.ld
  * @brief   Linker script for External Loader on STM32U5G9ZJT6Q
  * 
  * ★★★ CRITICAL ★★★
  * 1. StorageInfo MUST be at address 0x00000000
  * 2. StorageInfo size MUST be exactly 0xC8 (200 bytes)
  * 3. Code section starts at 0x20000000 (RAM)
  ******************************************************************************
  */

/* Entry Point - not needed for external loader */
/* ENTRY(Reset_Handler) */

/* Force linker to keep these required symbols */
EXTERN(StorageInfo)
EXTERN(Init)
EXTERN(DeInit)
EXTERN(Write)
EXTERN(Read)
EXTERN(SectorErase)
EXTERN(MassErase)
EXTERN(Verify)

/* Stack and heap configuration */
_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x400;

/* Memory regions
 * INFO: StorageInfo structure at address 0x00000000
 * RAM:  Code and data at 0x20000000 (STM32U5G9 SRAM1)
 */
MEMORY
{
  INFO (r)     : ORIGIN = 0x00000000, LENGTH = 0x200     /* StorageInfo region */
  RAM  (xrw)   : ORIGIN = 0x20000000, LENGTH = 0x280000  /* 2.5MB SRAM */
}

/* Output sections */
SECTIONS
{
  /* =========================================================================
   * StorageInfo Structure - MUST be at address 0x00000000
   * STM32CubeProgrammer reads device info from this location
   * ========================================================================= */
  .storage_info 0x00000000 :
  {
    . = ALIGN(4);
    KEEP(*(.storage_info))
    KEEP(*Dev_Inf.o(.rodata*))
    . = ALIGN(4);
  } > INFO

  /* =========================================================================
   * Code Section - Loaded into RAM at 0x20000000
   * ========================================================================= */
  .text :
  {
    . = ALIGN(4);
    _stext = .;
    
    *(.text)
    *(.text*)
    
    /* Keep external loader entry points */
    KEEP(*(.text.Init))
    KEEP(*(.text.DeInit))
    KEEP(*(.text.Write))
    KEEP(*(.text.Read))
    KEEP(*(.text.SectorErase))
    KEEP(*(.text.MassErase))
    KEEP(*(.text.Verify))
    
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;
  } > RAM

  /* Read-only data */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } > RAM

  /* ARM exception handling */
  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > RAM
  
  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > RAM

  /* Initialization arrays */
  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > RAM
  
  PROVIDE(_init = 0);
  PROVIDE(_fini = 0);
  
  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > RAM
  
  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > RAM

  /* Data section */
  _sidata = LOADADDR(.data);

  .data : 
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM

  /* BSS section (uninitialized data) */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } > RAM

  /* User heap and stack */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM

  /* Discard unused library sections to reduce size */
  /DISCARD/ :
  {
    libc.a(*)
    libm.a(*)
    libgcc.a(*)
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
