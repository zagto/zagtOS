#include <unistd.h>
#include <errno.h>
#include <zagtos/unixcompat.h>
#include "syscall.h"

ssize_t write(int fd, const void *buf, size_t count)
{
    ZFileDescriptor *zfd = zagtos_get_file_descriptor_object(fd);
    if (!zfd) {
        errno = EBADF;
        return -1;
    }

    if (zfd->is_syslog) {
        zagtos_syscall(SYS_LOG, count, buf);
        return count;
    }

    // TODO: write to file or socket
    errno = -ENOSYS;
    return -1;
}
