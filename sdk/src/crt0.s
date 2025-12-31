.section .text.init
.global _start

_start:
    # Initialize stack pointer
    la sp, _stack_top

    # Zero out BSS
    la a0, _bss_start
    la a1, _bss_end
    bgeu a0, a1, 2f
1:
    sw x0, 0(a0)
    addi a0, a0, 4
    bltu a0, a1, 1b
2:

    # Call main
    call main

    # Infinite loop if main returns
loop:
    j loop
