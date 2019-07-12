#include <unistd.h>
#include <errno.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"

int isatty(int fd)
{
    ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(fd);
    if (!zfd) {
        errno = EBADF;
        return -1;
    }

    if (zagtos_compare_uuid(zfd->object->info.type, TYPE_TTY)) {
        return 1;
    } else {
        errno = ENOTTY;
        return 0;
    }
}
