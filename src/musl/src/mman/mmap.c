#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <zagtos/KernelApi.h>
#include "syscall.h"

void *__mmap(void *start, size_t len, int prot, int flags, int fd, off_t off)
{
	if (len >= PTRDIFF_MAX) {
		errno = ENOMEM;
		return MAP_FAILED;
    }
    struct ZoMmapArguments args = {
        .startAddress = start,
        .offset = off,
        .length = len,
        .protection  = prot,
        .handle = fd,
        .flags = flags
    };
    uint32_t error = zagtos_syscall(SYS_MMAP, &args);
    if (error != 0) {
        errno = error;
        return MAP_FAILED;
    }
    return args.result;
}

weak_alias(__mmap, mmap);

weak_alias(mmap, mmap64);
