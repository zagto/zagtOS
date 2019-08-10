; Simple implementation of Spinlocks
; heavily influenced by:
; https://wiki.osdev.org/Spinlock


global basicLock
global basicUnlock

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


basicUnlock:
    mov qword [rdi], 0
    ret
