.global returnFromInterrupt
.global syscallEntry
.global ExceptionVectorTable
.global InKernelReturnEntry

.extern handleInterrupt
.extern InKernelReturnEntryRestoreInterruptsLock
.extern Syscall
.extern SyscallScheduleNext

.struct 0
RegisterState.fromSyscall:
    .struct RegisterState.fromSyscall + 8
RegisterState.pstate:
    .struct RegisterState.pstate + 8
RegisterState.pc:
    .struct RegisterState.pc + 8
RegisterState.lr:
    .struct RegisterState.lr + 8
RegisterState.sp:
    .struct RegisterState.sp + 8
RegisterState.exceptionType:
    .struct RegisterState.exceptionType + 8
RegisterState.exceptionSyndrome:
    .struct RegisterState.exceptionSyndrome + 8
RegisterState.dummy:
    .struct RegisterState.dummy + 8
RegisterState.x:
    .struct RegisterState.x + 8 * 30
RegisterState.q:
    .struct RegisterState.q + 16 * 32
RegisterState.end:

.section ".text"

.balign 0x1000
ExceptionVectorTable:
# EL1T syncronous
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x00
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    b commonExceptionVector
# EL1T IRQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x01
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    b commonExceptionVector
# EL1T FIQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x02
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    b commonExceptionVector
# EL1T error
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x03
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    b commonExceptionVector
# EL1H syncronous
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x10
    str x18, [sp, RegisterState.exceptionType]
    mov x18, sp
    add x18, x18, RegisterState.end
    b commonExceptionVector
# EL1H IRQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x11
    str x18, [sp, RegisterState.exceptionType]
    mov x18, sp
    add x18, x18, RegisterState.end
    b commonExceptionVector
# EL1H FIQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x12
    str x18, [sp, RegisterState.exceptionType]
    mov x18, sp
    add x18, x18, RegisterState.end
    b commonExceptionVector
# EL1H error
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x13
    str x18, [sp, RegisterState.exceptionType]
    mov x18, sp
    add x18, x18, RegisterState.end
    b commonExceptionVector
# EL0 64-bit syncronous
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]

    # This might be a syscall -> check exceptionClass
    mrs x18, esr_el1
    lsr x18, x18, 26
    and x18, x18, 0b111111
    cmp x18, 0b010101
    beq syscallEntry

    mov x18, 0x20
    str x18, [sp, RegisterState.exceptionType]
    mrs x18, sp_el0
    b commonExceptionVector
# EL0 64-bit IRQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x21
    str x18, [sp, RegisterState.exceptionType]
    mrs x18, sp_el0
    b commonExceptionVector
# EL0 64-bit FIQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x22
    str x18, [sp, RegisterState.exceptionType]
    mrs x18, sp_el0
    b commonExceptionVector
# EL0 64-bit error
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x23
    str x18, [sp, RegisterState.exceptionType]
    mrs x18, sp_el0
    b commonExceptionVector
# EL0 32-bit syncronous
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x30
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    b commonExceptionVector
# EL0 32-bit IRQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x31
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    b commonExceptionVector
# EL0 32-bit FIQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x32
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    b commonExceptionVector
# EL0 32-bit error
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x33
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    b commonExceptionVector

syscallEntry:
    mrs x18, elr_el1
    str x18, [sp, RegisterState.pc]
    str lr, [sp, RegisterState.lr]
    mrs x18, sp_el0
    str x18, [sp, RegisterState.sp]
    mov x18, 1
    str x18, [sp, RegisterState.fromSyscall]
    mrs x18, spsr_el1
    str x18, [sp, RegisterState.pstate]
    bl Syscall

    # ACTION_SCHEDULE
    cmp x0, 3
    beq scheduleAction

    # ACTION_CONTINUE should be the only other option to reach here
    cmp x0, 2
    bne badAction

    # continue to user space
    ldr x18, [sp, RegisterState.pc]
    msr elr_el1, x18
    ldr lr, [sp, RegisterState.lr]
    ldr x18, [sp, RegisterState.sp]
    msr sp_el0, x18
    ldr x18, [sp, RegisterState.pstate]
    msr spsr_el1, x18
    ldp x18, x19, [sp, RegisterState.x + 18 * 8]

    # load return value
    ldr x0, [sp, RegisterState.x + 0]

    add sp, sp, RegisterState.end
    eret

scheduleAction:
    # save callee-saved registers into our structure
    stp x20, x21, [sp, RegisterState.x + 20 * 8]
    stp x22, x23, [sp, RegisterState.x + 22 * 8]
    stp x24, x25, [sp, RegisterState.x + 24 * 8]
    stp x26, x27, [sp, RegisterState.x + 26 * 8]
    stp x28, x29, [sp, RegisterState.x + 28 * 8]

    stp q8, q9, [sp, RegisterState.q + 8 * 16]
    stp q10, q11, [sp, RegisterState.q + 10 * 16]
    stp q12, q13, [sp, RegisterState.q + 12 * 16]
    stp q14, q15, [sp, RegisterState.q + 14 * 16]

    # should not return
    bl SyscallScheduleNext

badAction:
    wfe
    b badAction

# x18 - user sp
commonExceptionVector:
    str x18, [sp, RegisterState.sp]
    mrs x18, esr_el1
    str x18, [sp, RegisterState.exceptionSyndrome]
    mov x18, 0
    str x18, [sp, RegisterState.fromSyscall]
    mrs x18, spsr_el1
    str x18, [sp, RegisterState.pstate]
    mrs x18, elr_el1
    str x18, [sp, RegisterState.pc]

    # save general-purpose registers
    str lr, [sp, RegisterState.lr]
    stp x0, x1, [sp, RegisterState.x + 0 * 8]
    stp x2, x3, [sp, RegisterState.x + 2 * 8]
    stp x4, x5, [sp, RegisterState.x + 4 * 8]
    stp x6, x7, [sp, RegisterState.x + 6 * 8]
    stp x8, x9, [sp, RegisterState.x + 8 * 8]
    stp x10, x11, [sp, RegisterState.x + 10 * 8]
    stp x12, x13, [sp, RegisterState.x + 12 * 8]
    stp x14, x15, [sp, RegisterState.x + 14 * 8]
    stp x16, x17, [sp, RegisterState.x + 16 * 8]
    # x18 and x19 were already saved while inside the ExceptionVectorTable
    stp x20, x21, [sp, RegisterState.x + 20 * 8]
    stp x22, x23, [sp, RegisterState.x + 22 * 8]
    stp x24, x25, [sp, RegisterState.x + 24 * 8]
    stp x26, x27, [sp, RegisterState.x + 26 * 8]
    stp x28, x29, [sp, RegisterState.x + 28 * 8]

    # save vector registers
    stp q2, q3, [sp, RegisterState.q + 0 * 16]
    stp q2, q3, [sp, RegisterState.q + 2 * 16]
    stp q4, q5, [sp, RegisterState.q + 4 * 16]
    stp q6, q7, [sp, RegisterState.q + 6 * 16]
    stp q8, q9, [sp, RegisterState.q + 8 * 16]
    stp q10, q11, [sp, RegisterState.q + 10 * 16]
    stp q12, q13, [sp, RegisterState.q + 12 * 16]
    stp q14, q15, [sp, RegisterState.q + 14 * 16]
    stp q16, q17, [sp, RegisterState.q + 16 * 16]
    stp q18, q19, [sp, RegisterState.q + 18 * 16]
    stp q20, q21, [sp, RegisterState.q + 20 * 16]
    stp q22, q23, [sp, RegisterState.q + 22 * 16]
    stp q24, q25, [sp, RegisterState.q + 24 * 16]
    stp q26, q27, [sp, RegisterState.q + 26 * 16]
    stp q28, q29, [sp, RegisterState.q + 28 * 16]
    stp q30, q31, [sp, RegisterState.q + 30 * 16]

    # place a pointer to the saved RegisterState in x0
    mov x0, sp

    b handleInterrupt
    # handleInterrupt should never return here

InKernelReturnEntry:
    mov sp, x0
    bl InKernelReturnEntryRestoreInterruptsLock
    mov x0, sp

returnFromInterrupt:
    # switch to state-save-stack passed as parameter
    mov sp, x0

    ldr x0, [sp, RegisterState.pc]
    msr elr_el1, x0

    ldr x1, [sp, RegisterState.fromSyscall]
    ldr x0, [sp, RegisterState.pstate]
    msr spsr_el1, x0
    # tst x0, #1 // FLAG_EL1H sets lowest bit
    and x0, x0, #1
    cmp x0, #0
    bne inKernelReturn

    # TODO: can skip restoring caller-saved registers when coming from syscall

    # to-user return:
    ldr x0, [sp, RegisterState.sp]
    msr sp_el0, x0

inKernelReturn:

    ldr lr, [sp, RegisterState.lr]
    ldp x0, x1, [sp, RegisterState.x + 0 * 8]
    ldp x2, x3, [sp, RegisterState.x + 2 * 8]
    ldp x4, x5, [sp, RegisterState.x + 4 * 8]
    ldp x6, x7, [sp, RegisterState.x + 6 * 8]
    ldp x8, x9, [sp, RegisterState.x + 8 * 8]
    ldp x10, x11, [sp, RegisterState.x + 10 * 8]
    ldp x12, x13, [sp, RegisterState.x + 12 * 8]
    ldp x14, x15, [sp, RegisterState.x + 14 * 8]
    ldp x16, x17, [sp, RegisterState.x + 16 * 8]
    ldp x18, x19, [sp, RegisterState.x + 18 * 8]
    ldp x20, x21, [sp, RegisterState.x + 20 * 8]
    ldp x22, x23, [sp, RegisterState.x + 22 * 8]
    ldp x24, x25, [sp, RegisterState.x + 24 * 8]
    ldp x26, x27, [sp, RegisterState.x + 26 * 8]
    ldp x28, x29, [sp, RegisterState.x + 28 * 8]

    ldp q2, q3, [sp, RegisterState.q + 0 * 16]
    ldp q2, q3, [sp, RegisterState.q + 2 * 16]
    ldp q4, q5, [sp, RegisterState.q + 4 * 16]
    ldp q6, q7, [sp, RegisterState.q + 6 * 16]
    ldp q8, q9, [sp, RegisterState.q + 8 * 16]
    ldp q10, q11, [sp, RegisterState.q + 10 * 16]
    ldp q12, q13, [sp, RegisterState.q + 12 * 16]
    ldp q14, q15, [sp, RegisterState.q + 14 * 16]
    ldp q16, q17, [sp, RegisterState.q + 16 * 16]
    ldp q18, q19, [sp, RegisterState.q + 18 * 16]
    ldp q20, q21, [sp, RegisterState.q + 20 * 16]
    ldp q22, q23, [sp, RegisterState.q + 22 * 16]
    ldp q24, q25, [sp, RegisterState.q + 24 * 16]
    ldp q26, q27, [sp, RegisterState.q + 26 * 16]
    ldp q28, q29, [sp, RegisterState.q + 28 * 16]
    ldp q30, q31, [sp, RegisterState.q + 30 * 16]

    add sp, sp, RegisterState.end
    eret

registerStateFromSyscallButNotUser:
    wfe
    b registerStateFromSyscallButNotUser

