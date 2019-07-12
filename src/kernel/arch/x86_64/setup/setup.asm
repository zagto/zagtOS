global switchStack

%define PAGE_SIZE 0x1000

section .text

switchStack:
    add rdi, PAGE_SIZE
    mov rsp, rdi
    mov rdi, rdx
    sub rsp, 8 ; alignment matters!
    push rsi
    ret
