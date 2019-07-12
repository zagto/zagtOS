#include <time.h>
#include "syscall.h"

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    return zagtos_syscall(SYS_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, req);
}
