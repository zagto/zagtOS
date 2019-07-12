#include <time.h>
#include "syscall.h"

int clock_getres(clockid_t clk, struct timespec *ts)
{
    /* Zagtos clocks have Microsecond-resolution */
    ts->tv_sec = 0;
    ts->tv_nsec = 1000;
}
