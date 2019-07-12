#include <sys/mman.h>
#include <errno.h>
#include "libc.h"
#include "syscall.h"

int __mprotect(void *addr, size_t len, int prot)
{
	size_t start, end;
	start = (size_t)addr & -PAGE_SIZE;
	end = (size_t)((char *)addr + len + PAGE_SIZE-1) & -PAGE_SIZE;

    int res = zagtos_syscall(SYS_MPROTECT, start, end-start, prot);
    if (res) {
        errno = res;
        return -1;
    } else {
        return 0;
    }
}

weak_alias(__mprotect, mprotect);
