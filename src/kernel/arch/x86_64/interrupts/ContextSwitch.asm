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
extern SyscallScheduleNext
extern kernelStackEnd
extern InKernelReturnEntryRestoreInterruptsLock


section .text


%define XSAVE_SSE (1<<1)
%define XSAVE_X87 (1<<0)

%define FSBASE_MSR 0xC0000100
%define GSBASE_MSR 0xC0000101
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

; Beginning of the CommonProcessor class we use in syscallEntry
struc CommonProcessor
    .self: resq 1
    .activeThread: resq 1
    .userRegisterState: resq 1
endstruc

struc RegisterState
    .fromSyscall resq 1
    .dummy resq 1

    .r15 resq 1
    .r14 resq 1
    .r13 resq 1
    .r12 resq 1
    .rbp resq 1
    .rbx resq 1

    .r11 resq 1
    .r10 resq 1
    .r9 resq 1
    .r8 resq 1

    .rdi resq 1
    .rsi resq 1

    .rdx resq 1
    .rcx resq 1
    .rax resq 1

    .intNr resq 1
    .errorCode resq 1

    .rip resq 1
    .cs resq 1
    .rflags resq 1
    .rsp resq 1
    .ss resq 1
endstruc

%define ACTION_RETRY 1
%define ACTION_CONTINUE 2
%define ACTION_SCHEDULE 3

syscallEntry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax

    ; get CurrentProcessor pointer for register save from kernel gsbase
    swapgs

    ; get RegisterState pointer into rax
    mov rax, [gs:CommonProcessor.userRegisterState]

    ; r15-rbx are callee-saved, only need to backup when scheduling away
    ; r11-errorCode no need to save

    mov [rax+RegisterState.rip], rcx
    mov qword [rax+RegisterState.cs], 0x20|3
    mov [rax+RegisterState.rflags], r11
    mov [rax+RegisterState.rsp], rsp
    mov qword [rax+RegisterState.ss], 0x18|3

    ; fromSyscall = 1
    mov qword [rax+RegisterState.fromSyscall], 1

    ; the kernel stack starts with the saved RegisterState
    mov rsp, rax

    ; syscall instruction overwrites rcx so this variable put into r10 by user space
    mov rcx, r10

    call Syscall

    cmp rax, ACTION_SCHEDULE
    je scheduleAction

    cmp rax, ACTION_CONTINUE
    jne badAction

    add rsp, 16*8 ;RegisterState.r10 ; continue with poping r10

    swapgs

    mov ax, 0x18|3
    mov ds, ax
    mov es, ax
    mov gs, ax

    jmp fromSyscallDirectly

scheduleAction:
    ; save callee-saved registers into our structure
    mov [rsp+RegisterState.r15], r15
    mov [rsp+RegisterState.r14], r14
    mov [rsp+RegisterState.r13], r13
    mov [rsp+RegisterState.r12], r12
    mov [rsp+RegisterState.rbp], rbp
    mov [rsp+RegisterState.rbx], rbx

    call SyscallScheduleNext

badAction:
    jmp badAction

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

    ; dummy = 0
    push 0
    ; fromSyscall = 0
    push 0

    ; place a pointer to the saved RegisterState in rdi
    mov rdi, rsp

    ; set all data segments except gs to avoid clearing GS_BASE on in-kernel interrupts
    mov ax, 0x10
    mov ds, ax
    mov es, ax

    ; switch to real stack if coming from user space interrupt
    ; check if the cs on stack is 0x20|3
    cmp qword [rdi+RegisterState.cs], 0x08
    je alreadyOnKernelStack

    cmp qword [rdi+RegisterState.cs], 0x20|3
    jne wrongCsOnEntry ; should never happen

    mov gs, ax

    ; CurrentProcessor pointer is in KERNEL_GS_BASE register while in user space and should be in
    ; GS_BASE while in kernel space
    swapgs

    ; rsp/rdi should now point to the userRegisterState variable of the CommonProcessor structure
    cmp rdi, [gs:CommonProcessor.userRegisterState]
    jne wrongRspOnEntry

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
    cmp qword [rsp+RegisterState.cs], 0x08
    je inKernelReturn
    cmp qword [rsp+RegisterState.cs], 0x20|3
    jne wrongCsOnReturn ; should never happen

    ; to-user return:
    swapgs

    mov ax, 0x18|3
    mov ds, ax
    mov es, ax
    mov gs, ax

inKernelReturn:
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
fromSyscallDirectly:
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


