[bits 64]

global InB
global OutB

section .text

; di: port to read from
InB:
    mov dx, di
    in al, dx
    ret

; di: port to write to
; si low: value to write
OutB:
    mov ax, si
    mov dx, di
    out dx, al
    ret
