/*
 * Linker script for the MX25L6433F external loader (.stldr).
 * Everything is linked into RAM - STM32CubeProgrammer loads the ELF
 * sections into the G0B1's SRAM and calls Init/Write/... directly.
 * Mirrors the structure of the proven MX25L12833F_Loader RAM script.
 */

ENTRY(Init)

/* Force linker to keep the external loader API symbols */
EXTERN(StorageInfo)
EXTERN(Init)
EXTERN(Write)
EXTERN(Read)
EXTERN(SectorErase)
EXTERN(MassErase)
EXTERN(Verify)

_estack = ORIGIN(RAM) + LENGTH(RAM);

_Min_Heap_Size  = 0x200;
_Min_Stack_Size = 0x400;

/* ST external-loader convention:
 *  - StorageInfo descriptor in its own section at address 0x00000000
 *    (CubeProgrammer reads it from the ELF, it is never loaded to the MCU)
 *  - code/data in RAM starting at +4 (first word reserved for the tool) */
MEMORY
{
  INFO (r)  : ORIGIN = 0x00000000, LENGTH = 0x200
  RAM (xrw) : ORIGIN = 0x20000004, LENGTH = 0x23FFC
}

SECTIONS
{
  .storage_info 0x00000000 :
  {
    . = ALIGN(4);
    KEEP(*(.storage_info))
    KEEP(*Dev_Inf*(.rodata*))
    . = ALIGN(4);
  } > INFO

  /* ST loader convention: code first, no vector table */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >RAM

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >RAM

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >RAM
  .ARM : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } >RAM

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM

  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
