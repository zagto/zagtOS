#include <sys/mman.h>
#include <errno.h>
#include "syscall.h"

#include <stdio.h>

int __munmap(void *start, size_t len)
{
    int ret = zagtos_syscall(SYS_MUNMAP, 0, start, len);
    if (ret != 0) {
        errno = ret;
        return -1;
    }
    return 0;
}

weak_alias(__munmap, munmap);
