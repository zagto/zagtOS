#include <stdio.h>
#include <syscall.h>
#include <zagtos/interrupt.h>

void zagtos_register_interrupt(uint64_t irq) {
    zagtos_syscall(SYS_REGISTER_INTERRUPT, irq);
}

void zagtos_unregister_interrupt(uint64_t irq) {
    zagtos_syscall(SYS_UNREGISTER_INTERRUPT, irq);
}
