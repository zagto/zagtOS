#include <time.h>
#include <errno.h>
#include <stdint.h>
#include "syscall.h"
#include "atomic.h"

int __clock_gettime(clockid_t clk, struct timespec *ts)
{
    if (clk < 0 || clk >= 4) {
        errno = EINVAL;
        return -1;
    }
    zagtos_syscall(SYS_CLOCK_GETTIME, clk, ts);
    return 0;
}

weak_alias(__clock_gettime, clock_gettime);
