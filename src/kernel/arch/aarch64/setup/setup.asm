.global switchStack

.section ".text"

switchStack:
    mov sp, x0
    mov x0, x2
    br x1
