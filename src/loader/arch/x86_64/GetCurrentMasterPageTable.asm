[bits 64]

global GetCurrentMasterPageTable

GetCurrentMasterPageTable:
    mov rax, cr2
    ret
