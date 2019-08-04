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
    uint32_t error;
    uint32_t flags;
    size_t offset;
    ZUUID target;
    void *result;
    uint32_t protection;
};

void *__mmap(void *start, size_t len, int prot, int flags, int fd, off_t off)
{
	if (len >= PTRDIFF_MAX) {
		errno = ENOMEM;
		return MAP_FAILED;
    }

    ZFileDescriptor *zfd = NULL;
    if (!(flags & MAP_ANON) && !(flags & MAP_PHYSICAL)) {
        zfd = zagtos_get_file_descriptor_object(fd);
        if (!zfd) {
            errno = EBADF;
            return MAP_FAILED;
        }
        /* Wo don't support executing code this way */
        if (prot & PROT_EXEC) {
            errno = EACCES;
            return MAP_FAILED;
        }
        if ((prot & PROT_WRITE) && !(prot & MAP_PRIVATE) && !zfd->write) {
            errno = EACCES;
            return MAP_FAILED;
        }
    }

    struct mmap_args args = {
        .start_address = start,
        .offset = off,
        .length = len,
        .protection  = prot,
        .flags = flags
    };
    if (zfd) {
        args.target = zfd->object->info.id;
    }

    zagtos_syscall(SYS_MMAP, &args);
    if (args.error) {
        errno = args.error;
        return MAP_FAILED;
    }
    return args.result;
}

weak_alias(__mmap, mmap);

weak_alias(mmap, mmap64);
