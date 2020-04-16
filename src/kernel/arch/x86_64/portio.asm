[bits 64]

global InB
global OutB
global InW
global OutW
global InD
global OutD

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

; di: port to read from
InW:
    mov dx, di
    in ax, dx
    ret

; di: port to write to
; si: value to write
OutW:
    mov ax, si
    mov dx, di
    out dx, ax
    ret

; di: port to read from
InD:
    mov dx, di
    in eax, dx
    ret

; di: port to write to
; esi: value to write
OutD:
    mov eax, esi
    mov dx, di
    out dx, eax
    ret
