.global returnFromInterrupt
.global syscallEntry
.global ExceptionVectorTable
.global InKernelReturnEntry

.extern handleInterrupt
.extern InKernelReturnEntryRestoreInterruptsLock

.struct 0
RegisterState.fromSyscall:
    .struct RegisterState.fromSyscall + 8
RegisterState.fromUser:
    .struct RegisterState.fromUser + 8
RegisterState.pstate:
    .struct RegisterState.pstate + 8
RegisterState.pc:
    .struct RegisterState.pc + 8
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
RegisterState.v:
    .struct RegisterState.v + 8 * 32
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
    mov x19, 0
    b commonExceptionVector
# EL1T IRQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x01
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    mov x19, 0
    b commonExceptionVector
# EL1T FIQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x02
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    mov x19, 0
    b commonExceptionVector
# EL1T error
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x03
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    mov x19, 0
    b commonExceptionVector
# EL1H syncronous
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x10
    str x18, [sp, RegisterState.exceptionType]
    mov x18, sp
    add x18, x18, RegisterState.end
    mov x19, 0
    b commonExceptionVector
# EL1H IRQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x11
    str x18, [sp, RegisterState.exceptionType]
    mov x18, sp
    add x18, x18, RegisterState.end
    mov x19, 0
    b commonExceptionVector
# EL1H FIQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x12
    str x18, [sp, RegisterState.exceptionType]
    mov x18, sp
    add x18, x18, RegisterState.end
    mov x19, 0
    b commonExceptionVector
# EL1H error
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x13
    str x18, [sp, RegisterState.exceptionType]
    mov x18, sp
    add x18, x18, RegisterState.end
    mov x19, 0
    b commonExceptionVector
# EL0 64-bit syncronous
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x20
    str x18, [sp, RegisterState.exceptionType]
    mrs x18, sp_el0
    mov x19, 1
    b commonExceptionVector
# EL0 64-bit IRQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x21
    str x18, [sp, RegisterState.exceptionType]
    mrs x18, sp_el0
    mov x19, 1
    b commonExceptionVector
# EL0 64-bit FIQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x22
    str x18, [sp, RegisterState.exceptionType]
    mrs x18, sp_el0
    mov x19, 1
    b commonExceptionVector
# EL0 64-bit error
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x23
    str x18, [sp, RegisterState.exceptionType]
    mrs x18, sp_el0
    mov x19, 1
    b commonExceptionVector
# EL0 32-bit syncronous
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x30
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    mov x19, 1
    b commonExceptionVector
# EL0 32-bit IRQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x31
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    mov x19, 1
    b commonExceptionVector
# EL0 32-bit FIQ
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x32
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    mov x19, 1
    b commonExceptionVector
# EL0 32-bit error
.balign 0x80
    sub sp, sp, RegisterState.end
    stp x18, x19, [sp, RegisterState.x + 18 * 8]
    mov x18, 0x33
    str x18, [sp, RegisterState.exceptionType]
    mov x18, 0
    mov x19, 1
    b commonExceptionVector

# x18 - user sp
# x19 - fromUser
commonExceptionVector:
    str x18, [sp, RegisterState.sp]
    str x19, [sp, RegisterState.fromUser]
    mrs x18, esr_el1
    str x18, [sp, RegisterState.exceptionSyndrome]
    mov x18, 0
    str x18, [sp, RegisterState.fromSyscall]
    mrs x18, spsr_el1
    str x18, [sp, RegisterState.pstate]
    mrs x18, elr_el1
    str x18, [sp, RegisterState.pc]

    # save general-purpose registers
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
    stp v0, v1, [sp, RegisterState.v + 0 * 8]
    stp v2, v3, [sp, RegisterState.v + 2 * 8]
    stp v4, v5, [sp, RegisterState.v + 4 * 8]
    stp v6, v7, [sp, RegisterState.v + 6 * 8]
    stp v8, v9, [sp, RegisterState.v + 8 * 8]
    stp v10, v11, [sp, RegisterState.v + 10 * 8]
    stp v12, v13, [sp, RegisterState.v + 12 * 8]
    stp v14, v15, [sp, RegisterState.v + 14 * 8]
    stp v16, v17, [sp, RegisterState.v + 16 * 8]
    stp v18, v19, [sp, RegisterState.v + 18 * 8]
    stp v20, v21, [sp, RegisterState.v + 20 * 8]
    stp v22, v23, [sp, RegisterState.v + 22 * 8]
    stp v24, v25, [sp, RegisterState.v + 24 * 8]
    stp v26, v27, [sp, RegisterState.v + 26 * 8]
    stp v28, v29, [sp, RegisterState.v + 28 * 8]
    stp v30, v31, [sp, RegisterState.v + 30 * 8]

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

    ldr x0, [sp, RegisterState.fromSyscall]
    ldr x1, [sp, RegisterState.fromUser]
    cmp x1, 1
    beq inKernelReturn

    # TODO: can skip restoring caller-saved registers when coming from syscall

    # to-user return:
    ldr x0, [sp, RegisterState.sp]
    msr sp_el0, x0

inKernelReturn:
    ldr x0, [sp, RegisterState.pstate]
    msr spsr_el1, x0

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

    ldp v0, v1, [sp, RegisterState.v + 0 * 8]
    ldp v2, v3, [sp, RegisterState.v + 2 * 8]
    ldp v4, v5, [sp, RegisterState.v + 4 * 8]
    ldp v6, v7, [sp, RegisterState.v + 6 * 8]
    ldp v8, v9, [sp, RegisterState.v + 8 * 8]
    ldp v10, v11, [sp, RegisterState.v + 10 * 8]
    ldp v12, v13, [sp, RegisterState.v + 12 * 8]
    ldp v14, v15, [sp, RegisterState.v + 14 * 8]
    ldp v16, v17, [sp, RegisterState.v + 16 * 8]
    ldp v18, v19, [sp, RegisterState.v + 18 * 8]
    ldp v20, v21, [sp, RegisterState.v + 20 * 8]
    ldp v22, v23, [sp, RegisterState.v + 22 * 8]
    ldp v24, v25, [sp, RegisterState.v + 24 * 8]
    ldp v26, v27, [sp, RegisterState.v + 26 * 8]
    ldp v28, v29, [sp, RegisterState.v + 28 * 8]
    ldp v30, v31, [sp, RegisterState.v + 30 * 8]

    add sp, sp, RegisterState.end
    eret

registerStateFromSyscallButNotUser:
    wfe
    b registerStateFromSyscallButNotUser

syscallEntry:
    b syscallEntry
