.global CurrentProcessor
.global InitCurrentProcessorPointer
.global basicIdleProcessor

.section ".text"

CurrentProcessor:
    mrs x0, TPIDR_EL1
    ret

InitCurrentProcessorPointer:
    msr TPIDR_EL1, x0
    ret

basicIdleProcessor:
    wfe
    b basicIdleProcessor
