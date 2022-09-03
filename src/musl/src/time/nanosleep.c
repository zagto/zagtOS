#include <time.h>
#include "syscall.h"

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    return zagtos_syscall(SYS_CLOCK_NANOSLEEP, TIMER_ABSTIME, CLOCK_MONOTONIC, req, rem);
}
