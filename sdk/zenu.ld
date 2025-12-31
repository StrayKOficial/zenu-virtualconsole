OUTPUT_ARCH( "riscv" )
ENTRY(_start)

MEMORY
{
  rom (rx)  : ORIGIN = 0x00010000, LENGTH = 16M
  ram (rwx) : ORIGIN = 0x01000000, LENGTH = 16M
}

SECTIONS
{
  .text : {
    *(.text.init)
    *(.text .text.*)
  } > rom

  .rodata : {
    *(.rodata .rodata.*)
  } > rom

  .data : {
    _data_start = .;
    *(.data .data.*)
    _data_end = .;
  } > ram AT > rom

  .bss : {
    _bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
    _bss_end = .;
  } > ram

  _stack_top = ORIGIN(ram) + LENGTH(ram);
}
