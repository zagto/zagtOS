#include <unistd.h>
#include <errno.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"

ssize_t read(int fd, void *buf, size_t count)
{
    ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(fd);
    if (!zfd) {
        errno = EBADF;
        return -1;
    }

    if (zfd->is_syslog) {
        return 0;
    }

    // TODO: read from file or socket
    errno = -ENOSYS;
    return -1;
}
