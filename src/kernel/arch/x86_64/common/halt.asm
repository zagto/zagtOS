global basicHalt

section .text

basicHalt:
    hlt
    jmp basicHalt
