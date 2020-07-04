; Simple implementation of Spinlocks
; heavily influenced by:
; https://wiki.osdev.org/Spinlock


global basicLock
global basicUnlock
global basicTrylock

section .text


basicLock:
    lock bts qword [rdi], 0
    jc .spin
    ret
.spin:
    pause
    test qword [rdi], 1
    jnz .spin
    jmp basicLock


basicTrylock:
    mov rax, 1
    lock bts qword [rdi], 0
    jc .fail
    ret
.fail:
    mov rax, 0
    ret


basicUnlock:
    mov qword [rdi], 0
    ret
