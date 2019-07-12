global random

section .text

random:
    ;rdrand eax
    ;jnc random
    mov eax, 42
    ret
