[bits 64]

global readTimerValue

section .text

readTimerValue:
    rdtsc
    shl rdx, 32
    or rax, rdx
    ret
