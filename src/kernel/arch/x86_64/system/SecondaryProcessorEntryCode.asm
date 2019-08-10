global SecondaryProcessorEntryCode
global SecondaryProcessorEntryCodeEnd
global SecondaryProcessorEntryMasterPageTable

extern KernelEntrySecondaryProcessor

; Yes, this code is in the data section.
; It should not be executed directly, but rather copied into a yery low memory location, so the
; processor can access it when staring in real mode
section .data

%define MSR_EFER 0xc0000080
%define EFER_LME 0x100
%define EFER_NXE 0x800

%define CR0_PROTECTED_MODE 1
%define CR0_COPROCESSOR_EMULATION 4
%define CR0_COPROCESSOR_MONITORING 2
%define CR0_PAGING 0x80000000

%define CR4_PAE (1 << 5)
%define CR4_PGE (1 << 7)
%define CR4_OSFXSR (1 << 9)
%define CR4_OSXMMEXCPT (1 << 10)

[bits 16]

SecondaryProcessorEntryCode:
entry:
    ; ebx will contain the address where SecondaryProcessorEntryCode will be put at
    ; actual value will be inserted by C code
    mov ebx, 0x12345678


    ; patch flushCSJump longModeJump to jump to actual address this is loaded at
    mov cx, bx
    add cx, longMode - entry
    mov bp, bx
    add bp, longModeJump - entry
    ; the destination address is in the second and third byte of the jmp instruction
    mov [bp+1], cx

    mov cx, bx
    add cx, flushCS - entry
    mov bp, bx
    add bp, flushCSJump - entry
    ; the destination address is in the second and third byte of the jmp instruction
    mov [bp+1], cx

flushCSJump:
    jmp 0x00:0x1234


flushCS:
    ; enable Longmode and SSE
    ; based on: https://wiki.osdev.org/Entering_Long_Mode_Directly
    ; and: https://wiki.osdev.org/SSE
    mov eax, cr0
    and eax, ~CR0_COPROCESSOR_EMULATION
    or eax, CR0_COPROCESSOR_MONITORING
    ;mov cr0, eax
    mov eax, cr4
    or eax, CR4_OSFXSR | CR4_OSXMMEXCPT | CR4_PAE | CR4_PGE
    mov cr4, eax

    ; load the MPT address to CR3
    mov bp, bx
    add bp, SecondaryProcessorEntryMasterPageTable - entry
    mov ecx, [bp]
    mov cr3, ecx


; set bits in EFER MSR to enable Long Mode and Non-Execute feature
    mov ecx, MSR_EFER
    rdmsr
    or eax, EFER_NXE | EFER_LME
    wrmsr

    mov ecx, cr0
    and ecx, ~CR0_COPROCESSOR_EMULATION
    or ecx, CR0_PAGING | CR0_PROTECTED_MODE | CR0_COPROCESSOR_MONITORING
    mov cr0, ecx

    mov bp, bx
    add bp, gdtr - entry
    mov ax, bx
    add ax, gdt - entry
    mov dword [bp+2], eax
    lgdt [bp]


longModeJump:
    jmp 0x08:0x1234

SecondaryProcessorEntryMasterPageTable:
    dd 0x12345678

gdt:
    dq 0
    dq 0x00209A0000000000
    dq 0x0000920000000000

gdtr:
    dw $ - gdt - 1
    dd 0x12345678


[bits 64]
longMode:
    mov ax, 0x10
    mov ds, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, rbx
    add rsp, 0x1000
    mov rax, KernelEntrySecondaryProcessor
    call rax

SecondaryProcessorEntryCodeEnd: