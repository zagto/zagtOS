.global TimerFrequency
.global detectTimerFrequency

.section ".text"
detectTimerFrequency:
    mrs x0, CNTFRQ_EL0
    adr x1, TimerFrequency
    str x0, [x1]
    ret

.section ".data"
.align 8
TimerFrequency:
    .quad 0
