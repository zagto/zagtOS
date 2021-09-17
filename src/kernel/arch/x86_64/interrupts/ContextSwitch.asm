[bits 64]

global InterruptServiceRoutines
global returnFromInterrupt
global syscallEntry
global KERNEL_STACK_SIZE
global basicDisableInterrupts
global basicEnableInterrupts
global InKernelReturnEntry

extern handleInterrupt
extern Syscall
extern kernelStackEnd
extern InKernelReturnEntryRestoreInterruptsLock


section .text


%define XSAVE_SSE (1<<1)
%define XSAVE_X87 (1<<0)

%define FSBASE_MSR 0xC0000100
%define KERNEL_GSBASE_MSR 0xC0000102

InterruptServiceRoutines:

; %1 - interrupt number
; %2 - has error code?
%macro createISR 1
%%stubStart:
    %if (%1 == 0x02 || %1 == 0x12)
        ; ignore MCE or NMI
        ; these have a special small stack! If you want do da more than ignoring here, change their size
        ; in Interrupts.cpp
        iretq
    %else
        ; if this interrupt has no error code, push 0 as replacement
        ; to keep the stack layout always the same
        %if !(%1 == 8 || (%1 >= 10 && %1 <= 14) || %1 == 17)
            push qword 0
        %endif

        ; push interrupt number on stack
        push qword %1

        jmp commonISR
    %endif

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
    mov [gs:0x20], r15
    mov [gs:0x28], r14
    mov [gs:0x30], r13
    mov [gs:0x38], r12
    mov [gs:0x40], rbp
    mov [gs:0x48], rbx
    ; r11-errorCode no need to save

    mov [gs:0xa8], rcx ; rip
    mov qword [gs:0xb0], 0x20|3 ; cs
    mov [gs:0xb8], r11 ; rflags
    mov [gs:0xc0], rsp
    mov qword [gs:0xc8], 0x18|3 ; ss

    ; fromSyscall = 1
    mov qword [gs:0x10], 1

    ; currentProcessor is the second field at the beginning of registerState
    ; this pointer is put in r15 (as the CurrentProcessor variable)
    mov r15, [gs:0x8]

    ; the kernel stack starts with the saved RegisterState
    mov rsp, [gs:0x0]

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

    ; dummy = 0
    push 0
    ; fromSyscall = 0
    push 0

    ; make rsp point to the exact beginning of registerState structure, 2 fields before
    sub rsp, 8*2
    ; place a pointer to the saved register state in rdi
    mov rdi, rsp

    ; switch to real stack if coming from user space interrupt
    ; check if the cs on stack is 0x20|3
    cmp qword [rsp+(22*8)], 0x08
    je alreadyOnKernelStack

    cmp qword [rsp+(22*8)], 0x20|3
    jne wrongCsOnEntry ; should never happen

    ; rsp should now point to the "self" variable of the RegisterState structure
    cmp rsp, [rsp]
    jne wrongRspOnEntry

    ; currentProcessor is the second field at the beginning of registerState
    ; this pointer is put in r15 (as the CurrentProcessor variable)
    mov r15, [rdi+8]
alreadyOnKernelStack:
    call handleInterrupt
    ; handler should never return here and call returnFromInterrupt instead

handleInterruptReturned:
    jmp handleInterruptReturned
wrongCsOnEntry:
    jmp wrongCsOnEntry
wrongCsOnReturn:
    jmp wrongCsOnReturn
wrongRspOnEntry:
    jmp wrongRspOnEntry

InKernelReturnEntry:
    mov rsp, rdi
    call InKernelReturnEntryRestoreInterruptsLock
    mov rdi, rsp

returnFromInterrupt:
    ; switch to state-save-stack passed as parameter
    mov rsp, rdi

commonReturn:
    ; switch to real stack if coming from user space interrupt
    ; check if the cs on stack is 0x20|3
    cmp qword [rsp+(22*8)], 0x08
    je inKernelReturn
    cmp qword [rsp+(22*8)], 0x20|3
    jne wrongCsOnReturn ; should never happen

    ; to-user return:

    mov ax, 0x18|3
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


inKernelReturn:
    ; ignore currentProcessor, self
    add rsp, 8*2

    ; if the fromSyscall field is set we can skip restoring caller-saved registers
    pop rax

    ; ignore dummy
    add rsp, 8

    cmp rax, 1
    je fromSyscall

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
    ; callee-saved registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ; ignore r11-rcx
    add rsp, 8*8
    xor r10, r10
    xor r10, r10
    xor r9, r9
    xor r8, r8
    xor rdi, rdi
    xor rsi, rsi

    pop rax ; return value
    add rsp, 2*8 ; intNr, errorCode

    pop rcx ; rip
    add rsp, 8 ; cs
    pop r11 ; rflags
    pop rsp
    o64 sysret


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

basicDisableInterrupts:
    cli
    ret

basicEnableInterrupts:
    sti
    ret


