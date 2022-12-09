.global readTimerValue

.section ".text"
readTimerValue:
    mrs x0, CNTPCT_EL0
    ret
