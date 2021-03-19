[bits 64]

global compare_exchange_i32

section .text

; inputs:
; rdi - pointer to variable to change
; rsi - pointer to expected value - changes if it does not match
; edx - desired
; output:
; rax - 0 = failure, 1 = success
compare_exchange_i32:
compare_exchange_u32:
    mov eax, [rsi]
    lock cmpxchg [rdi], edx
    mov [rsi], eax
    jnz .fail
    mov rax, 1
    ret
.fail:
    mov rax, 0
    ret
