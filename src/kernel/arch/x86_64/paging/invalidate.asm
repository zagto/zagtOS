global basicInvalidate
global basicSwitchMasterPageTable

section .text

basicInvalidate:
    invlpg [rdi]
    ret

basicSwitchMasterPageTable:
mov rax, rdi
    mov cr3, rax
    ret
