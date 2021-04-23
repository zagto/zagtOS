#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"

struct mmap_args {
    void *start_address;
    size_t length;
    uint32_t flags;
    size_t offset;
    void *result;
    uint32_t handle;
    uint32_t protection;
};

void *__mmap(void *start, size_t len, int prot, int flags, int fd, off_t off)
{
	if (len >= PTRDIFF_MAX) {
		errno = ENOMEM;
		return MAP_FAILED;
    }

    struct mmap_args args = {
        .start_address = start,
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
