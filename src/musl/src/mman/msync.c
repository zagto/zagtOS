#include <sys/mman.h>
#include <errno.h>
#include "syscall.h"

int msync(void *start, size_t len, int flags)
{
    int ret = zagtos_syscall(SYS_MSYNC, start, len, flags);
    if (ret) {
        errno = ret;
        return -1;
    }
    return 0;
}
