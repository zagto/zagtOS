#include <string.h>
#include <stdint.h>
#include <zagtos/syscall.h>
#include "pthread_impl.h"

uintptr_t __stack_chk_guard;

void __init_ssp(void)
{
    zagtos_syscall(SYS_RANDOM, sizeof(uintptr_t), &__stack_chk_guard);
    pthread_self()->CANARY = __stack_chk_guard;
}

void __stack_chk_fail(void)
{
	a_crash();
}

hidden void __stack_chk_fail_local(void);

weak_alias(__stack_chk_fail, __stack_chk_fail_local);
