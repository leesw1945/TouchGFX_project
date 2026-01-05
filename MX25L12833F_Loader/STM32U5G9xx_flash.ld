/**
  ******************************************************************************
  * @file    STM32U5G9xx_flash.ld
  * @brief   Linker script for External Loader on STM32U5G9ZJT6Q
  * 
  * CRITICAL: StorageInfo MUST be placed at address 0x00000000 for 
  *           STM32CubeProgrammer to parse correctly.
  *           Code goes to RAM starting at 0x20000004.
  ******************************************************************************
  */

/* Entry Point - not used for external loader */
/* ENTRY(Reset_Handler) */

/* Force linker to keep these symbols */
EXTERN(StorageInfo)
EXTERN(Init)
EXTERN(Write)
EXTERN(Read)
EXTERN(SectorErase)
EXTERN(MassErase)
EXTERN(Verify)

/* Stack pointer */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Heap and stack sizes */
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x400;

/* Memory areas */
/* CRITICAL: INFO region at address 0 for StorageInfo */
MEMORY
{
  INFO (r)     : ORIGIN = 0x00000000, LENGTH = 0x400      /* StorageInfo at address 0 */
  RAM (xrw)    : ORIGIN = 0x20000004, LENGTH = 0x27FFC    /* Code in RAM - STM32U5G9 has 2.5MB SRAM */
}

/* Output sections */
SECTIONS
{
  /* =========================================================================
   * StorageInfo MUST be at address 0x00000000
   * STM32CubeProgrammer expects this exact location
   * ========================================================================= */
  .storage_info 0x00000000 :
  {
    . = ALIGN(4);
    KEEP(*(.storage_info))
    KEEP(*Dev_Inf.o(.rodata*))
    . = ALIGN(4);
  } >INFO

  /* Code section in RAM */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    
    /* Keep external loader functions */
    KEEP(*(.text.Init))
    KEEP(*(.text.Write))
    KEEP(*(.text.Read))
    KEEP(*(.text.SectorErase))
    KEEP(*(.text.MassErase))
    KEEP(*(.text.Verify))
    
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >RAM

  /* Constant data in RAM */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >RAM

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >RAM
  
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >RAM

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >RAM
  
  PROVIDE(_init = 0);
  PROVIDE(_fini = 0);
  
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >RAM
  
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >RAM

  _sidata = LOADADDR(.data);

  /* Initialized data */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM

  /* Uninitialized data */
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
  } >RAM

  /* Heap and stack */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Discard unused library sections */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
