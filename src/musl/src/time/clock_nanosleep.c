#include <time.h>
#include <errno.h>
#include "syscall.h"

int __clock_nanosleep(clockid_t clk, int flags, const struct timespec *req, struct timespec *rem)
{
    if (clk < 0 || clk >= 4 || req->tv_nsec < 0 || req->tv_nsec >= 1000 * 1000 * 1000) {
        errno = EINVAL;
        return -1;
    }
    zagtos_syscall(SYS_CLOCK_NANOSLEEP, flags & TIMER_ABSTIME, clk, req);
    return 0;
}

weak_alias(__clock_nanosleep, clock_nanosleep);
