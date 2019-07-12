; Simple implementation of Spinlocks
; heavily influenced by:
; https://wiki.osdev.org/Spinlock


global basicLock
global basicUnlock

section .text

basicLock:
    lock bts dword [rdi], 0
    jc .spin
    ret

.spin:
    pause
    test dword [rdi], 1
    jnz .spin
    jmp basicLock


basicUnlock:
    mov dword [rdi], 0
    ret
