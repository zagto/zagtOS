[bits 64]

global ExitFinalize

section .text


; ExitFinalize - this function does:
; - setup Non-Execute support
; - switch to a temporary stack that is part of the loader data and therefore stay mapped during
;   the jump into kernel
; - switch to a temporary GDT for the same reason
; - activate the new master page table to map the kernel to it's correct virtual address
; - jump to kernel entry
;
; All data structures are defined in C code and pointers will be given as parameters to this
; function. This is necessary because GNU EFI's relocation hacks won't work in this assempbly code.
;
; Parameters:
; - rdi: virtual address of the kernel entry
; - rsi: master page table to activate
; - rdx: kernel boot info (this pointer will be passed directly to kernel)
; - rcx: temporary stack
; - r8: processorID
; - r9: hardwareID

%define SERIAL_PORT 0x3f8

%define MSR_EFER 0xc0000080
%define EFER_NXE 0x800
%define EFER_SCE 0x1

%define MSR_PAT 0x277
%define PAT_WRITE_BACK 6
%define PAT_UNCACHABLE 0
%define PAT_WRITE_COMBINING 1
%define PAT_WRITE_THROUGH 4

%define CR0_COPROCESSOR_EMULATION 4
%define CR0_COPROCESSOR_MONITORING 2

%define CR4_OSFXSR (1 << 9)
%define CR4_OSXMMEXCPT (1 << 10)


ExitFinalize:
    cli
    mov rsp, rcx

    ; backup rdx
    mov r10, rdx
    mov r11, rsi

    ; enable SSE (based on: https://wiki.osdev.org/SSE)
    mov rax, cr0
    and rax, ~CR0_COPROCESSOR_EMULATION
    or rax, CR0_COPROCESSOR_MONITORING
    mov cr0, rax
    mov rax, cr4
    or rax, CR4_OSFXSR | CR4_OSXMMEXCPT
    mov cr4, rax

    ; set NXE bit in EFER MSR to enable Non-Execute and Syscall instruction features
    mov rcx, MSR_EFER
    rdmsr
    or rax, EFER_NXE | EFER_SCE
    wrmsr

    ; set up PAT to correspond to the cache types defined in paging.h
    mov rcx, MSR_PAT
    mov rax, (PAT_WRITE_BACK) | (PAT_UNCACHABLE << 8) | (PAT_WRITE_THROUGH << 16) | (PAT_WRITE_COMBINING << 24)
    wrmsr

    ; switch to new master page table
    mov cr3, r11

    ; correctly mis-align stack
    push 0
    push rdi
    mov rdi, r10
    mov rsi, r8
    mov rdx, r9
    ret
