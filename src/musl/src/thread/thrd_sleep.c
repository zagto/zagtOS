#include <threads.h>
#include <errno.h>
#include <time.h>
#include "syscall.h"

int thrd_sleep(const struct timespec *req, struct timespec *rem)
{
    int ret = nanosleep(req, rem);
    return ret ? -2 : 0;
}
