extern _code
extern _bss
extern _end
extern LoaderMain
extern _init

global _start
global __cxa_atexit
global MultibootInfo

%define MSR_EFER 0xc0000080
%define EFER_LME 0x100
%define EFER_NXE 0x800
%define EFER_SCE 0x1

%define MSR_PAT 0x277
%define PAT_WRITE_BACK 6
%define PAT_UNCACHABLE 0
%define PAT_WRITE_COMBINING 1
%define PAT_WRITE_THROUGH 4

%define CR0_PROTECTED_MODE 1
%define CR0_COPROCESSOR_EMULATION 4
%define CR0_COPROCESSOR_MONITORING 2
%define CR0_PAGING 0x80000000

%define CR4_PAE (1 << 5)
%define CR4_PGE (1 << 7)
%define CR4_OSFXSR (1 << 9)
%define CR4_OSXMMEXCPT (1 << 10)
%define CR4_OSXSAVE (1 << 18)

section .text:
[bits 32]
_start:
    jmp _start

    mov [MultibootInfo], ebx

    ; enable Longmode and SSE
    ; based on: https://wiki.osdev.org/Entering_Long_Mode_Directly
    ; and: https://wiki.osdev.org/SSE
    mov eax, cr0
    and eax, ~CR0_COPROCESSOR_EMULATION
    or eax, CR0_COPROCESSOR_MONITORING
    ;mov cr0, eax                        <-- TODO ???????????????????????????????????????
    mov eax, cr4
    or eax, CR4_OSFXSR | CR4_OSXMMEXCPT | CR4_PAE | CR4_PGE
    mov cr4, eax

    ; load the MPT address to CR3
    mov ecx, pageMapLevel4
    mov cr3, ecx


    ; set bits in EFER MSR to enable Long Mode and Non-Execute and Syscall instruction features
    mov ecx, MSR_EFER
    rdmsr
    or eax, EFER_NXE | EFER_LME | EFER_SCE
    wrmsr

    ; set up PAT to correspond to the cache types defined in CacheType
    mov ecx, MSR_PAT
    mov eax, (PAT_WRITE_BACK) | (PAT_UNCACHABLE << 8) | (PAT_WRITE_THROUGH << 16) | (PAT_WRITE_COMBINING << 24)
    wrmsr
    xor eax, eax

    mov ecx, cr0
    and ecx, ~CR0_COPROCESSOR_EMULATION
    or ecx, CR0_PAGING | CR0_PROTECTED_MODE | CR0_COPROCESSOR_MONITORING
    mov cr0, ecx


    lgdt [gdtr]

    longModeJump:
    jmp 0x08:longMode

[bits 64]
longMode:
    mov ax, 0x10
    mov ds, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, stackEnd

    ; Call global constructors
    call _init

    call LoaderMain

.neverReached:
    jmp .neverReached

__cxa_atexit:
    ; ignore __cxa_atexit
    mov rax, 0
    ret

section .multiboot:
MultibootHeader:
    dd 0xe85250d6       ; magic
    dd 0                ; architecture = x86
    dd (MultibootHeaderEnd - MultibootHeader) ; size
    dd - (MultibootHeaderEnd - MultibootHeader) - 0xe85250d6 ; checksum
MultibootInfoTag:
    dw 1                ; tag type
    dw 0                ; flags
    dd (Multiboot2Tag - MultibootInfoTag)
    dd 8                ; Framebuffer
    dd 6                ; Memory Map
    dd 14               ; ACPI old
    dd 15               ; ACPI new
    dd 3                ; Modules
    dd 0                ; end
Multiboot2Tag:
    dw 2
    dw 0
    dd (MultibootEndTag - Multiboot2Tag)
    dd MultibootHeader  ; Header Address
    dd MultibootHeader  ; Load Address
    dd _bss             ; Load End Address
    dd _end             ; BSS End Address
MultibootEndTag:
    dw 0                ; tag type
    dw 0                ; flags
    dd (MultibootHeaderEnd - MultibootEndTag)
MultibootHeaderEnd:


section .bss
MultibootInfo:
    resq 0 ; Pointer to Multiboot Info Stucture passed by Multiboot loader

stack:
    resb 0x1000
stackEnd:

section .data
gdt:
    dq 0
    dq 0x00209A0000000000
    dq 0x0000920000000000

gdtr:
    dw $ - gdt - 1
    dd gdt

pageMapLevel4:
    ; 256GiB Physical Addresses max.
    %define NUM_PML4_ENTRIES 1
    %define ENTRIES_PER_TABLE 0x1000/8
    %define TWO_M 2*1024*1024

    %define PAGE_PRESENT 0x1
    %define PAGE_NO_CACHE 0x08 ; PAT, was 0x10
    %define PAGE_HUGE 0x800

    %assign index 0
    %rep NUM_PML4_ENTRIES
        dq (pageDirectoryPointerTable + index * 0x1000) + (PAGE_PRESENT | PAGE_NO_CACHE)
        %assign index index+1
    %endrep
    %rep ENTRIES_PER_TABLE - NUM_PML4_ENTRIES
        dq 0
    %endrep

    pageDirectoryPointerTable:
    %assign tableIndex 0
    %rep NUM_PML4_ENTRIES
        %assign entryIndex 0
        %rep ENTRIES_PER_TABLE
            dq (pageDirectory + tableIndex * ENTRIES_PER_TABLE * 0x1000 + entryIndex * 0x1000) + (PAGE_PRESENT | PAGE_NO_CACHE)
            %assign entryIndex entryIndex+1
        %endrep
        %assign tableIndex tableIndex+1
    %endrep

    pageDirectory:
    %assign tableIndex 0
    %rep NUM_PML4_ENTRIES * ENTRIES_PER_TABLE
        %assign entryIndex 0
        %rep ENTRIES_PER_TABLE
            dq (tableIndex * TWO_M * ENTRIES_PER_TABLE + entryIndex * TWO_M) | PAGE_HUGE | PAGE_PRESENT | PAGE_NO_CACHE
            %assign entryIndex entryIndex+1
        %endrep
        %assign tableIndex tableIndex+1
    %endrep
