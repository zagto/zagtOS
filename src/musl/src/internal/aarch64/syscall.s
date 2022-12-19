.global zagtos_syscall
.global zagtos_syscall0
.global zagtos_syscall1
.global zagtos_syscall2
.global zagtos_syscall3
.global zagtos_syscall4
.global zagtos_syscall5
.type zagtos_syscall,@function
.type zagtos_syscall0,@function
.type zagtos_syscall1,@function
.type zagtos_syscall2,@function
.type zagtos_syscall3,@function
.type zagtos_syscall4,@function
.type zagtos_syscall5,@function
zagtos_syscall:
zagtos_syscall0:
zagtos_syscall1:
zagtos_syscall2:
zagtos_syscall3:
zagtos_syscall4:
zagtos_syscall5:
    svc #0
    ret
