global basicInvalidate
global basicInvalidateTLBContext
global basicSwitchMasterPageTable

section .text

; rdi - TLB Context ID (0 until PCID support)
; rsi - address
basicInvalidateTLBContext:
    invlpg [rsi]
    ret

basicInvalidate:
    invlpg [rdi]
    ret

basicSwitchMasterPageTable:
    mov rax, rdi
    mov cr3, rax
    ret
