[bits 64]

global loadInterruptDescriptorTable
global loadTaskStateSegment
global basicIdleProcessor
global readCR2
global loadGlobalDescriptorTable
global setFSBase

section .text

loadInterruptDescriptorTable:
    lidt [rdi]
    ret

loadTaskStateSegment:
    mov rax, rdi
    mov rdx, 16 ; size of 1 TSS entry
    mul rdx
    add rax, 0x28 ; first TSS entry
    or rax, 3
    ltr ax
    ret

basicIdleProcessor:
    ;sti
    hlt
    ret

readCR2:
    mov rax, cr2
    ret

loadGlobalDescriptorTable:
    lgdt [rdi]

    push 0x08
    call setCS

    mov rax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ret

setCS:
    ; this magic instruction was found in the UEFI loader of the Essence operating system
    ; https://bitbucket.org/nakst/essence
    ; file: essence/boot/x86/uefi_loader.s
    db 0x48, 0xcb

%define FSBASE_MSR 0xC0000100

