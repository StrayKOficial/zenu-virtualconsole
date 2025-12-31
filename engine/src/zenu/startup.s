.section .text.init
.global _start
_start:
    # Initialize stack pointer
    la sp, _stack_top
    
    # Clear BSS
    la a0, _bss_start
    la a1, _bss_end
    bge a0, a1, skip_bss
clear_bss:
    sw zero, 0(a0)
    addi a0, a0, 4
    blt a0, a1, clear_bss
skip_bss:

    # Jump to C++ main
    jal ra, main

    # Halt if main returns
loop:
    j loop
