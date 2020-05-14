#define _GNU_SOURCE
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "syscall.h"
#include "atomic.h"

int __clock_gettime(clockid_t clk, struct timespec *ts)
{
    if (clk < 0 || clk >= 2) {
        errno = EINVAL;
        fprintf(stderr, "warning: attempt to read clock %i which is not supported on Zagtos\n", clk);
        return -1;
    }
    zagtos_syscall(SYS_CLOCK_GETTIME, clk, ts);

    /* real time clock is not initialized on early boot */
    if (ts->tv_sec == 0 && ts->tv_nsec == 0) {
        assert(clk == CLOCK_REALTIME);
        errno = ENOTSUP;
        return -1;
    }
    return 0;
}

weak_alias(__clock_gettime, clock_gettime);
