global switchStack

extern KERNEL_STACK_SIZE

%define PAGE_SIZE 0x1000

section .text

switchStack:
    mov rsp, rdi
    mov rdi, rdx
    ;sub rsp, 8 ; alignment matters!
    push rsi
    ret
