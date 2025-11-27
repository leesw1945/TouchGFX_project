/*
******************************************************************************
* @file      STM32U5G9ZJTXQ_RAM.ld
* @brief     Linker script for External Loader (RAM execution)
*            STM32U5G9ZJTxQ Device - 2MB SRAM
******************************************************************************
*/

/* Entry Point - MUST be Init for External Loader */
ENTRY(Init)

/* Memory areas - All in RAM */
MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 2048K
}

/* Sections */
SECTIONS
{
  /* Device Info structure - MUST be at fixed location */
  .dev_info 0x20000004 :
  {
    KEEP(*(.rodata.StorageInfo))
    KEEP(*(.rodata.Dev_Inf*))
    . = ALIGN(4);
  } > RAM
  
  /* Program code */
  .text :
  {
    . = ALIGN(4);
    _stext = .;
    
    /* Loader API functions - Keep them! */
    KEEP(*(.text.Init))
    KEEP(*(.text.DeInit))
    KEEP(*(.text.Write))
    KEEP(*(.text.Read))
    KEEP(*(.text.SectorErase))
    KEEP(*(.text.MassErase))
    KEEP(*(.text.CheckSum))
    KEEP(*(.text.Verify))
    
    *(.text)
    *(.text*)
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

  /* ARM sections */
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

  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > RAM

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

  /* Initialized data */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM

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
  } > RAM

  /* Heap and Stack */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + 0x400;   /* Heap: 1KB */
    . = . + 0x800;   /* Stack: 2KB */
    . = ALIGN(8);
  } > RAM

  /* Discard */
  /DISCARD/ :
  {
    libc.a(*)
    libm.a(*)
    libgcc.a(*)
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
