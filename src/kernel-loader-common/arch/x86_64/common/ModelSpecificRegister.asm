[bits 64]

global readModelSpecificRegister
global writeModelSpecificRegister

section .text

readModelSpecificRegister:
    mov ecx, edi
    rdmsr
    shl rdx, 32
    or rax, rdx
    ret

writeModelSpecificRegister:
    mov ecx, edi
    mov eax, esi
    shr rsi, 32
    mov edx, esi
    wrmsr
    ret
