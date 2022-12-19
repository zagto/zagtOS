.global returnFromInterrupt
.global syscallEntry

.section ".text"

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
RegisterState.x:
    .struct RegisterState.x + 8 * 30


returnFromInterrupt:
    b returnFromInterrupt
    # switch to state-save-stack passed as parameter
    mov sp, x0
    ldr x0, [sp, RegisterState.fromSyscall]
    ldr x1, [sp, RegisterState.fromUser]
    cmp x1, 1
    beq inKernelReturn

    # to-user return:
    ldr x2, [sp, RegisterState.spsr_el0]
    msr spsr_el1, x2

registerStateFromSyscallButNotUser:
    wfe
    b registerStateFromSyscallButNotUser

syscallEntry:
    b syscallEntry
