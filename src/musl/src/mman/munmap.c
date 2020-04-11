#include <sys/mman.h>
#include <errno.h>
#include "syscall.h"

int __munmap(void *start, size_t len)
{
    int ret = zagtos_syscall(SYS_MUNMAP, start, len);
    if (ret == 0) {
        errno = ret;
        return -1;
    }
    return 0;
}

weak_alias(__munmap, munmap);
