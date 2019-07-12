[bits 64]

global InterruptServiceRoutines
global returnFromInterrupt

extern handleInterrupt
extern kernelStackEnd


section .text

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
    ; pad some zeros to keep the code exactly 10 bytes
    ; since the size all stubs is hardcoded in IDT building code
    times (12 - (%%stubEnd - %%stubStart)) db 0
%endmacro

    %assign i 0
    %rep 256
        createISR i
        %assign i i+1
    %endrep

commonISR:
    ; save general-purpose registers
    push rax
    push rbx
    push rcx
    push rdx

    push rsi
    push rdi
    push rbp
    ; rsp was already saved

    push r8
    push r9
    push r10
    push r11

    push r12
    push r13
    push r14
    push r15

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; place a pointer to the saved register state in rdi
    mov rdi, rsp

    ; switch to real stack if coming from user space exception
    ; check if the cs on stack is 0x18|3
    cmp qword [rsp+(18*8)], 0x18|3
    jne alreadyOnKernelStack

    ; currentProcessor is the field before _registerState (which is align(16))
    ; this pointer is put in r15 (as the CurrentProcessor variable)
    mov r15, [rdi - 16]
    ; kernelStack is the first field of Processor class
    mov rsp, [r15]
    ; start at the end of kernelStack region
    add rsp, 0x1000

alreadyOnKernelStack:
    call handleInterrupt

    ; handler should never return here and call returnFromInterrupt instead


returnFromInterrupt:
    ; switch to state-save-stack passed as parameter
    mov rsp, rdi

    mov ax, 0x18|3
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

    pop r11
    pop r10
    pop r9
    pop r8

    ; rsp will be restored by iretq
    pop rbp
    pop rdi
    pop rsi

    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; pop interrupt number and error code
    add rsp, 2*8

    iretq

