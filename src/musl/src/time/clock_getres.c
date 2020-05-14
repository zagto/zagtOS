#include <time.h>
#include "syscall.h"

int clock_getres(clockid_t clk, struct timespec *ts)
{
    if (ts) {
        /* Zagtos clocks have highest resolution */
        ts->tv_sec = 0;
        ts->tv_nsec = 1;
    }
}
