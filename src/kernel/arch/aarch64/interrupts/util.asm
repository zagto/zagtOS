.global CurrentProcessor
.global InitCurrentProcessorPointer
.global basicIdleProcessor
.global SetUserTLSRegister
.global SetExceptionVectorTable

.section ".text"

CurrentProcessor:
    mrs x0, tpidr_el1
    ret

InitCurrentProcessorPointer:
    msr tpidr_el1, x0
    ret

SetUserTLSRegister:
    msr tpidr_el0, x0
    ret

basicIdleProcessor:
    wfe
    b basicIdleProcessor

SetExceptionVectorTable:
    msr vbar_el1, x0
    ret
