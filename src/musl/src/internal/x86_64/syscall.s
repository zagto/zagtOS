.global zagtos_syscall
.type zagtos_syscall,@function
zagtos_syscall:
    int $0xff
    ret
