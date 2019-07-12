#include <unistd.h>
#include <errno.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"

int dup(int fd)
{
    if (!zagtos_get_file_descriptor_object(fd)) {
        errno = EBADF;
        return -1;
    }

    int new = zagtos_clone_file_descriptor(fd, 3, 1);
    if (new < 0) {
        errno = EMFILE;
        return -1;
    }
    return new;
}
