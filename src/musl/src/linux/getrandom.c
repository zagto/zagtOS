#include <sys/random.h>
#include "syscall.h"

ssize_t getrandom(void *buf, size_t buflen, unsigned flags)
{
    zagtos_syscall(SYS_RANDOM, buf, buflen);
    return buflen;
}
