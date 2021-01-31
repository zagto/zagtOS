[bits 64]

global InterruptServiceRoutines
global returnFromInterrupt
global returnFromInKernelInterrupt
global syscallEntry
global KERNEL_STACK_SIZE

extern handleInterrupt
extern Syscall
extern kernelStackEnd


section .text


%define XSAVE_SSE (1<<1)
%define XSAVE_X87 (1<<0)

%define FSBASE_MSR 0xC0000100
%define KERNEL_GSBASE_MSR 0xC0000102

%define KERNEL_STACK_SIZE_DEF 0x1000

InterruptServiceRoutines:

; %1 - interrupt number
; %2 - has error code?
%macro createISR 1
%%stubStart:
    ; if this interrupt has no error code, push 0 as replacement
    ; to keep the stack layout always the same
    %if !(%1 == 8 || (%1 >= 10 && %1 <= 14) || %1 == 17)
        push qword 0
    %endif

    ; push interrupt number on stack
    push qword %1

    jmp commonISR

%%stubEnd:
    ; pad some zeros to keep the code exactly 12 bytes
    ; since the size all stubs is hardcoded in IDT building code
    times (12 - (%%stubEnd - %%stubStart)) db 0
%endmacro

    %assign i 0
    %rep 256
        createISR i
        %assign i i+1
    %endrep

syscallEntry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; get stack pointer for register save from kernel gsbase
    swapgs
    mov [gs:0x10], r15
    mov [gs:0x18], r14
    mov [gs:0x20], r13
    mov [gs:0x28], r12
    mov [gs:0x30], rbp
    mov [gs:0x38], rbx
    ; r11-errorCode no need to save

    mov [gs:0x98], rcx ; rip
    mov qword [gs:0xa0], 0x18|3 ; cs
    mov [gs:0xa8], r11 ; rflags
    mov [gs:0xb0], rsp
    mov qword [gs:0xb8], 0x20|3 ; ss

    ; fromSyscall = 1
    mov qword [gs:0x8], 1

    ; currentProcessor is the field at the beginning of registerState
    ; this pointer is put in r15 (as the CurrentProcessor variable)
    mov r15, [gs:0x0]
    ; kernelStack is the first field of Processor class
    mov rsp, [r15]
    ; start at the end of kernelStack region
    add rsp, KERNEL_STACK_SIZE_DEF

    ; syscall instruction overwrites rcx so this variable put into r10 by user space
    mov rcx, r10

    call Syscall
    ; should never retrun
loop0:
    jmp loop0

commonISR:
    ; save general-purpose registers
    push rax
    push rcx
    push rdx

    push rsi
    push rdi
    ; rsp was already saved

    push r8
    push r9
    push r10
    push r11

    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; fromSyscall = 0
    push 0

    ; make rsp point to the exact beginning of registerState structure, 1 field before
    sub rsp, 8
    ; place a pointer to the saved register state in rdi
    mov rdi, rsp

    ; switch to real stack if coming from user space exception
    ; check if the cs on stack is 0x18|3
    cmp qword [rsp+(20*8)], 0x08
    je alreadyOnKernelStack
    cmp qword [rsp+(20*8)], 0x18|3
    jne loop1 ; should never happen

    ; currentProcessor is the field at the beginning of registerState
    ; this pointer is put in r15 (as the CurrentProcessor variable)
    mov r15, [rdi]
    ; kernelStack is the first field of Processor class
    mov rsp, [r15]
    ; start at the end of kernelStack region
    add rsp, KERNEL_STACK_SIZE_DEF

alreadyOnKernelStack:
    call handleInterrupt
    ; handler should never return here and call returnFromInterrupt instead
loop1:
    jmp loop1


returnFromInterrupt:
    ; switch to state-save-stack passed as parameter
    mov rsp, rdi

    mov ax, 0x20|3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; user fsbase
    mov rax, rsi
    mov rdx, rsi
    shr rdx, 32
    mov rcx, FSBASE_MSR
    wrmsr

    ; save pointer to RegisterState end into kernel gsbase for use by syscall handler
    mov rax, rsp
    mov rdx, rax
    shr rdx, 32
    mov rcx, KERNEL_GSBASE_MSR
    wrmsr

    ; ignore currentProcessor
    add rsp, 8

    ; if the fromSyscall field is set we can skip restoring caller-saved registers
    pop rax
    cmp rax, 1
   ; je fromSyscall

    ; pop saved registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    pop r11
    pop r10
    pop r9
    pop r8

    pop rdi
    pop rsi

    pop rdx
    pop rcx
    pop rax

    ; pop interrupt number and error code
    add rsp, 2*8

    iretq


fromSyscall:
    ; switch to state-save-stack passed as parameter
    mov rsp, rdi

    mov ax, 0x20|3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    %define FSBASE_MSR 0xC0000100

    mov rax, rsi
    mov rdx, rsi
    shr rdx, 32
    mov rcx, FSBASE_MSR
    wrmsr

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    xor r10, r10
    xor r9, r9
    xor r8, r8

    xor rdi, rdi
    xor rsi, rsi

    add rax, 9*8

    ; save pointer to RegisterState end into cr2 for use by syscall handler
    mov rax, rsp
    add rax, 7*8 ; intNr-ss
    mov cr2, rax

    pop rdx ; return value
    add rsp, 2*8 ; intNr, errorCode

    pop rcx ; rip
    add rsp, 8 ; cs
    pop r11 ; rflags

    pop rsp

    mov ax, 0x20|3
    mov ss, ax

    mov rax, rdx ; return value
    o64 sysret


returnFromInKernelInterrupt:
    ; switch to state-save-stack passed as parameter
    mov rsp, rdi

    ; fromSyscall and currentProcessor fields (dummy for in-kernel interrupts)
    add rsp, 2*8

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    pop r11
    pop r10
    pop r9
    pop r8

    pop rdi
    pop rsi

    pop rdx
    pop rcx
    pop rax

    ; pop interrupt number and error code
    add rsp, 2*8

    iretq

saveVectorRegisters:
    ; for user-space, save vector registers
    sub rsp, 1024
    mov rdx, 0
    mov rax, XSAVE_SSE|XSAVE_X87
    xsave [rsp]

restoreVectorRegisters:
    mov rdx, 0
    mov rax, XSAVE_SSE|XSAVE_X87
    xrstor [rsp]
    add rsp, 1024


section .data
KERNEL_STACK_SIZE:
    dq KERNEL_STACK_SIZE_DEF

